/**
 * \file merge_detail.hpp
 * \brief Incremental merge of component `.log` files.
 *
 * Behavior follows `dist/logs/merge_incremental.py`. Callers own scheduling.
 * This header does not read pipes or `logger.ini`.
 *
 * `merge_once` holds `Local\AutoCore.logger.merge`. That mutex covers merge
 * state and publishing. Component log writes use their own destination lock.
 * A missing `merge.state` replaces Main logs from component logs.
 * `merge.journal` is written before those logs change so the next merge can
 * commit or undo an interrupted publish before it trusts saved offsets.
 *
 * Each pass snapshots source sizes before it reads. Bytes appended after
 * that snapshot stay unread until a later pass. After a detached shutdown
 * merge, that later pass may be the next Auto Core session.
 */
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace ac::logger::detail {

    struct MergeResult {
        bool ok = true;
        std::string message;
        bool stopped = false;
    };

    /** `on` and `off` only. Any other text, including a missing value, is rejected. */
    [[nodiscard]]
    inline std::optional<bool> parse_on_off(
        const std::optional<std::string_view> value
    ) noexcept {
        if (value == "on") {
            return true;
        }
        if (value == "off") {
            return false;
        }
        return std::nullopt;
    }

    /**
     * A non-negative whole number, including zero. The text must be consumed
     * completely. A missing, empty, negative, or partial value is rejected.
     */
    [[nodiscard]]
    inline std::optional<std::uint64_t> parse_interval(
        const std::optional<std::string_view> value
    ) noexcept {
        if (!value || value->empty()) {
            return std::nullopt;
        }
        std::int64_t parsed = 0;
        const auto* const end = value->data() + value->size();
        const auto result = std::from_chars(value->data(), end, parsed);
        if (result.ec != std::errc {} || result.ptr != end || parsed < 0) {
            return std::nullopt;
        }
        return static_cast<std::uint64_t>(parsed);
    }

    namespace {

        constexpr std::size_t timestamp_length = 23;

        [[nodiscard]]
        std::string casefold(std::string_view text) {
            std::string folded;
            folded.reserve(text.size());
            for (const unsigned char character : text) {
                folded.push_back(static_cast<char>(
                    std::tolower(character)
                ));
            }
            return folded;
        }

        [[nodiscard]]
        bool is_utf8(const std::string_view bytes) {
            const auto* cursor =
                reinterpret_cast<const unsigned char*>(bytes.data());
            const auto* const end = cursor + bytes.size();
            while (cursor < end) {
                if (*cursor <= 0x7F) {
                    ++cursor;
                    continue;
                }
                int extra = 0;
                if ((*cursor & 0xE0) == 0xC0) {
                    if (*cursor < 0xC2) {
                        return false;
                    }
                    extra = 1;
                }
                else if ((*cursor & 0xF0) == 0xE0) {
                    extra = 2;
                }
                else if ((*cursor & 0xF8) == 0xF0) {
                    if (*cursor > 0xF4) {
                        return false;
                    }
                    extra = 3;
                }
                else {
                    return false;
                }
                ++cursor;
                for (int index = 0; index < extra; ++index) {
                    if (cursor >= end || (*cursor & 0xC0) != 0x80) {
                        return false;
                    }
                    ++cursor;
                }
            }
            return true;
        }

        [[nodiscard]]
        std::string relative_key(
            const std::filesystem::path& log_directory,
            const std::filesystem::path& path
        ) {
            return path.lexically_relative(log_directory).generic_string();
        }

        [[nodiscard]]
        bool is_component_log_name(
            const std::string& name,
            std::string& date
        ) {
            constexpr std::string_view suffix = ".log";
            if (!name.ends_with(suffix) || name.size() < 11 + 1 + suffix.size()) {
                return false;
            }
            if (name[4] != '-' || name[7] != '-' || name[10] != '_') {
                return false;
            }
            for (const int index : {0, 1, 2, 3, 5, 6, 8, 9}) {
                if (name[static_cast<std::size_t>(index)] < '0' ||
                    name[static_cast<std::size_t>(index)] > '9') {
                    return false;
                }
            }
            date = name.substr(0, 10);
            return true;
        }

        struct SourceLog {
            std::filesystem::path path;
            std::string date;
            std::string component;
            std::string key;
        };

        [[nodiscard]]
        std::vector<SourceLog> discover_logs(
            const std::filesystem::path& log_directory
        ) {
            std::vector<SourceLog> logs;
            const auto components = log_directory / "components";
            std::error_code error;
            if (!std::filesystem::is_directory(components, error)) {
                return logs;
            }

            for (const auto& entry :
                 std::filesystem::recursive_directory_iterator(components, error)) {
                if (error || !entry.is_regular_file()) {
                    continue;
                }
                const std::string filename = entry.path().filename().string();
                std::string date;
                if (!is_component_log_name(filename, date)) {
                    continue;
                }
                // YYYY-MM-DD_ is the validated 11-character prefix. The tag
                // is the remainder except the trailing .log. Other filenames
                // stay undiscovered.
                constexpr std::string_view log_suffix = ".log";
                constexpr std::size_t date_prefix_length = 11;
                SourceLog log;
                log.path = entry.path();
                log.date = std::move(date);
                log.component = filename.substr(
                    date_prefix_length,
                    filename.size() - date_prefix_length - log_suffix.size()
                );
                log.key = relative_key(log_directory, entry.path());
                logs.push_back(std::move(log));
            }

            std::ranges::sort(logs, [](const SourceLog& left, const SourceLog& right) {
                return left.path.generic_string() < right.path.generic_string();
            });
            return logs;
        }

        [[nodiscard]]
        std::map<std::string, std::uint64_t> load_state(
            const std::filesystem::path& state_file
        ) {
            std::map<std::string, std::uint64_t> state;
            std::ifstream input(state_file);
            if (!input) {
                return state;
            }
            std::string line;
            while (std::getline(input, line)) {
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                if (line.empty() || line.starts_with('#')) {
                    continue;
                }
                const auto separator = line.rfind('=');
                if (separator == std::string::npos) {
                    continue;
                }
                try {
                    state.insert_or_assign(
                        line.substr(0, separator),
                        static_cast<std::uint64_t>(std::stoull(
                            line.substr(separator + 1)
                        ))
                    );
                }
                catch (const std::exception&) {
                    continue;
                }
            }
            return state;
        }

        [[nodiscard]]
        bool save_state(
            const std::filesystem::path& state_file,
            const std::map<std::string, std::uint64_t>& state
        ) {
            const auto temporary = state_file.wstring() + L".tmp";
            {
                std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
                if (!output) {
                    return false;
                }
                output << "# Auto Core main-log merge offsets\n";
                std::vector<std::pair<std::string, std::uint64_t>> rows(
                    state.begin(),
                    state.end()
                );
                std::ranges::sort(rows, [](const auto& left, const auto& right) {
                    return casefold(left.first) < casefold(right.first);
                });
                for (const auto& [key, offset] : rows) {
                    output << key << '=' << offset << '\n';
                }
                if (!output) {
                    return false;
                }
            }

            if (MoveFileExW(
                    temporary.c_str(),
                    state_file.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
                ) == 0) {
                return false;
            }
            return true;
        }

        constexpr std::uint64_t fnv_offset = 14695981039346656037ull;
        constexpr std::uint64_t fnv_prime = 1099511628211ull;
        constexpr wchar_t merge_mutex_name[] = L"Local\\AutoCore.logger.merge";

        [[nodiscard]]
        std::uint64_t fnv_feed(
            std::uint64_t hash,
            const std::string_view bytes
        ) {
            for (const unsigned char character : bytes) {
                hash ^= character;
                hash *= fnv_prime;
            }
            return hash;
        }

        [[nodiscard]]
        bool fnv_file(
            const std::filesystem::path& path,
            std::uint64_t& hash
        ) {
            std::ifstream input(path, std::ios::binary);
            if (!input) {
                return false;
            }
            hash = fnv_offset;
            char buffer[4096];
            while (input) {
                input.read(buffer, sizeof(buffer));
                const auto count = input.gcount();
                if (count > 0) {
                    hash = fnv_feed(
                        hash,
                        std::string_view {buffer, static_cast<std::size_t>(count)}
                    );
                }
            }
            return input.eof();
        }

        [[nodiscard]]
        std::filesystem::path journal_path(
            const std::filesystem::path& log_directory
        ) {
            return log_directory / "merge.journal";
        }

        enum class FilePresence {
            absent,
            regular,
            failed
        };

        // MSVC's status() reports "not found" through error_code. A missing
        // Main log is absent, not a failed read.
        [[nodiscard]]
        FilePresence regular_file(
            const std::filesystem::path& path,
            std::error_code& error
        ) {
            error.clear();
            if (std::filesystem::is_regular_file(path, error)) {
                error.clear();
                return FilePresence::regular;
            }
            if (!error || error == std::errc::no_such_file_or_directory) {
                error.clear();
                return FilePresence::absent;
            }
            return FilePresence::failed;
        }

        [[nodiscard]]
        bool journal_exists(const std::filesystem::path& log_directory) {
            std::error_code error;
            return regular_file(journal_path(log_directory), error) ==
                FilePresence::regular;
        }

        void remove_regular(const std::filesystem::path& path) {
            std::error_code error;
            if (regular_file(path, error) != FilePresence::regular) {
                return;
            }
            std::filesystem::remove(path, error);
        }

        [[nodiscard]]
        bool parse_u64(
            const std::string& text,
            std::uint64_t& value,
            const int base
        ) {
            if (text.empty()) {
                return false;
            }
            try {
                std::size_t consumed = 0;
                value = static_cast<std::uint64_t>(
                    std::stoull(text, &consumed, base)
                );
                return consumed == text.size();
            }
            catch (const std::exception&) {
                return false;
            }
        }

        [[nodiscard]]
        std::string hex64(const std::uint64_t value) {
            static constexpr char digits[] = "0123456789abcdef";
            std::string text(16, '0');
            std::uint64_t remaining = value;
            for (int index = 15; index >= 0; --index) {
                text[static_cast<std::size_t>(index)] =
                    digits[remaining & 0xF];
                remaining >>= 4;
            }
            return text;
        }

        [[nodiscard]]
        std::vector<std::string> split_tabs(const std::string& line) {
            std::vector<std::string> fields;
            std::size_t start = 0;
            while (start <= line.size()) {
                const auto tab = line.find('\t', start);
                if (tab == std::string::npos) {
                    fields.push_back(line.substr(start));
                    break;
                }
                fields.push_back(line.substr(start, tab - start));
                start = tab + 1;
            }
            return fields;
        }

        [[nodiscard]]
        bool safe_log_name(const std::string& name) {
            return name.ends_with("_main.log") &&
                name.find('\\') == std::string::npos &&
                name.find('/') == std::string::npos &&
                name.find("..") == std::string::npos;
        }

        [[nodiscard]]
        bool safe_state_key(const std::string& key) {
            if (key.empty() ||
                key.starts_with('/') ||
                key.starts_with('\\') ||
                key.find(':') != std::string::npos ||
                key.find("..") != std::string::npos) {
                return false;
            }
            return true;
        }

        struct JournalOutput {
            std::string filename;
            std::uint64_t previous_size = 0;
            std::uint64_t expected_size = 0;
            std::uint64_t hash_before = 0;
            std::uint64_t hash_after = 0;
            bool rebuild = false;
            bool existed = false;
            std::vector<std::pair<std::string, std::uint64_t>> offsets;
        };

        struct MergeLock {
            HANDLE handle = nullptr;
            bool abandoned = false;

            MergeLock() = default;
            MergeLock(const MergeLock&) = delete;
            MergeLock& operator=(const MergeLock&) = delete;

            MergeLock(MergeLock&& other) noexcept
                : handle(other.handle),
                  abandoned(other.abandoned) {
                other.handle = nullptr;
            }

            MergeLock& operator=(MergeLock&&) = delete;

            ~MergeLock() {
                if (handle != nullptr) {
                    ReleaseMutex(handle);
                    CloseHandle(handle);
                }
            }
        };

        enum class AcquireKind {
            owned,
            stopped,
            failed
        };

        struct AcquireOutcome {
            MergeLock lock;
            AcquireKind kind = AcquireKind::failed;
        };

        /**
         * `stop_event` makes the wait interruptible. A signaled stop event
         * does not acquire the mutex. A null stop event waits until the
         * mutex is owned. An abandoned mutex still grants ownership.
         */
        [[nodiscard]]
        AcquireOutcome acquire_merge_lock(const HANDLE stop_event) {
            AcquireOutcome outcome;
            outcome.lock.handle = CreateMutexW(nullptr, FALSE, merge_mutex_name);
            if (outcome.lock.handle == nullptr) {
                return outcome;
            }

            DWORD waited = WAIT_FAILED;
            if (stop_event == nullptr) {
                waited = WaitForSingleObject(outcome.lock.handle, INFINITE);
                if (waited == WAIT_OBJECT_0) {
                    outcome.kind = AcquireKind::owned;
                    return outcome;
                }
                if (waited == WAIT_ABANDONED_0) {
                    outcome.lock.abandoned = true;
                    outcome.kind = AcquireKind::owned;
                    return outcome;
                }
            }
            else {
                HANDLE handles[] {stop_event, outcome.lock.handle};
                waited = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                if (waited == WAIT_OBJECT_0) {
                    CloseHandle(outcome.lock.handle);
                    outcome.lock.handle = nullptr;
                    outcome.kind = AcquireKind::stopped;
                    return outcome;
                }
                if (waited == WAIT_OBJECT_0 + 1) {
                    outcome.kind = AcquireKind::owned;
                    return outcome;
                }
                if (waited == WAIT_ABANDONED_0 + 1) {
                    outcome.lock.abandoned = true;
                    outcome.kind = AcquireKind::owned;
                    return outcome;
                }
            }

            CloseHandle(outcome.lock.handle);
            outcome.lock.handle = nullptr;
            outcome.kind = AcquireKind::failed;
            return outcome;
        }

        [[nodiscard]]
        bool write_journal(
            const std::filesystem::path& path,
            const std::vector<JournalOutput>& outputs
        ) {
            const auto temporary = path.wstring() + L".tmp";
            {
                std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
                if (!output) {
                    return false;
                }
                output << "# Auto Core main-log merge journal\n";
                for (const auto& planned : outputs) {
                    output << "output\t" << planned.filename
                        << '\t' << planned.previous_size
                        << '\t' << planned.expected_size
                        << '\t' << hex64(planned.hash_before)
                        << '\t' << hex64(planned.hash_after)
                        << '\t' << (planned.rebuild ? "rebuild" : "append")
                        << '\t' << (planned.existed ? '1' : '0')
                        << '\n';
                    for (const auto& [key, offset] : planned.offsets) {
                        output << key << '=' << offset << '\n';
                    }
                }
                if (!output) {
                    return false;
                }
            }
            if (MoveFileExW(
                    temporary.c_str(),
                    path.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
                ) == 0) {
                return false;
            }
            return true;
        }

        [[nodiscard]]
        bool load_journal(
            const std::filesystem::path& path,
            std::vector<JournalOutput>& outputs,
            std::string& error
        ) {
            std::ifstream input(path, std::ios::binary);
            if (!input) {
                error = "Unable to read " + path.string();
                return false;
            }
            std::size_t current = static_cast<std::size_t>(-1);
            std::string line;
            while (std::getline(input, line)) {
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                if (line.empty() || line.starts_with('#')) {
                    continue;
                }
                if (line.starts_with("output\t")) {
                    const auto fields = split_tabs(line);
                    if (fields.size() != 8 ||
                        fields[0] != "output" ||
                        !safe_log_name(fields[1]) ||
                        (fields[6] != "append" && fields[6] != "rebuild") ||
                        (fields[7] != "0" && fields[7] != "1")) {
                        error = "Unable to read " + path.string();
                        return false;
                    }
                    JournalOutput planned;
                    planned.filename = fields[1];
                    if (!parse_u64(fields[2], planned.previous_size, 10) ||
                        !parse_u64(fields[3], planned.expected_size, 10) ||
                        !parse_u64(fields[4], planned.hash_before, 16) ||
                        !parse_u64(fields[5], planned.hash_after, 16)) {
                        error = "Unable to read " + path.string();
                        return false;
                    }
                    planned.rebuild = fields[6] == "rebuild";
                    planned.existed = fields[7] == "1";
                    outputs.push_back(std::move(planned));
                    current = outputs.size() - 1;
                    continue;
                }
                if (current >= outputs.size()) {
                    error = "Unable to read " + path.string();
                    return false;
                }
                const auto separator = line.rfind('=');
                if (separator == std::string::npos) {
                    error = "Unable to read " + path.string();
                    return false;
                }
                const auto key = line.substr(0, separator);
                std::uint64_t offset = 0;
                if (!safe_state_key(key) ||
                    !parse_u64(line.substr(separator + 1), offset, 10)) {
                    error = "Unable to read " + path.string();
                    return false;
                }
                outputs[current].offsets.emplace_back(key, offset);
            }
            return true;
        }

        struct Repair {
            bool ok = true;
            bool committed = false;
            std::string message;
        };

        [[nodiscard]]
        Repair repair_output(
            const std::filesystem::path& log_directory,
            const JournalOutput& planned
        ) {
            const auto destination = log_directory / planned.filename;
            remove_regular(
                std::filesystem::path {destination.wstring() + L".merging"}
            );

            std::error_code error;
            const auto presence = regular_file(destination, error);
            if (presence == FilePresence::failed) {
                return {false, false, "Unable to read " + destination.string()};
            }
            const bool present = presence == FilePresence::regular;

            if (planned.rebuild) {
                if (!present) {
                    return {};
                }
                std::uint64_t hash = 0;
                const auto size =
                    std::filesystem::file_size(destination, error);
                if (error || !fnv_file(destination, hash)) {
                    return {
                        false,
                        false,
                        "Unable to read " + destination.string()
                    };
                }
                if (hash == planned.hash_after &&
                    size == planned.expected_size) {
                    return {true, true, {}};
                }
                if (planned.existed && hash == planned.hash_before) {
                    return {};
                }
                return {
                    false,
                    false,
                    "Unable to recover " + destination.string()
                };
            }

            if (!present) {
                return {};
            }
            const auto size = std::filesystem::file_size(destination, error);
            if (error) {
                return {false, false, "Unable to read " + destination.string()};
            }
            if (size == planned.expected_size &&
                planned.expected_size > planned.previous_size) {
                return {true, true, {}};
            }
            if (planned.existed && size == planned.previous_size) {
                return {};
            }
            if (!planned.existed && size < planned.expected_size) {
                remove_regular(destination);
                return {};
            }
            if (planned.existed &&
                size > planned.previous_size &&
                size < planned.expected_size) {
                std::filesystem::resize_file(
                    destination,
                    planned.previous_size,
                    error
                );
                if (error) {
                    return {
                        false,
                        false,
                        "Unable to write " + destination.string()
                    };
                }
                return {};
            }
            return {
                false,
                false,
                "Unable to recover " + destination.string()
            };
        }

        [[nodiscard]]
        MergeResult recover_interrupted(
            const std::filesystem::path& log_directory
        ) {
            const auto path = journal_path(log_directory);
            if (!journal_exists(log_directory)) {
                return {};
            }

            std::vector<JournalOutput> outputs;
            std::string error;
            if (!load_journal(path, outputs, error)) {
                return {false, error};
            }

            const auto state_file = log_directory / "merge.state";
            auto state = load_state(state_file);
            bool dirty = false;
            for (const auto& planned : outputs) {
                const auto repaired = repair_output(log_directory, planned);
                if (!repaired.ok) {
                    return {false, repaired.message};
                }
                if (!repaired.committed) {
                    continue;
                }
                for (const auto& [key, offset] : planned.offsets) {
                    state.insert_or_assign(key, offset);
                }
                dirty = true;
            }

            if (dirty && !save_state(state_file, state)) {
                return {false, "Unable to write " + state_file.string()};
            }
            remove_regular(path);
            if (journal_exists(log_directory)) {
                return {false, "Unable to write " + path.string()};
            }
            return {};
        }

    } // namespace

    struct Record {
            std::string timestamp;
            std::string component_sort;
            std::string path_sort;
            std::uint64_t offset = 0;
            std::string line;
        };

        struct ReadResult {
            std::vector<Record> records;
            std::uint64_t consumed_offset = 0;
        };

        [[nodiscard]]
        ReadResult read_records(
            const std::filesystem::path& path,
            const std::string& component,
            const std::string& key,
            const std::uint64_t start_offset,
            const std::uint64_t end_offset
        ) {
            ReadResult result;
            result.consumed_offset = start_offset;
            std::ifstream input(path, std::ios::binary);
            if (!input) {
                return result;
            }
            input.seekg(static_cast<std::streamoff>(start_offset));
            if (!input) {
                return result;
            }

            std::string raw;
            std::uint64_t position = start_offset;
            std::uint64_t line_start = start_offset;
            bool crossed = false;
            while (position < end_offset) {
                char character = '\0';
                if (!input.get(character)) {
                    break;
                }
                ++position;
                if (character != '\n') {
                    raw.push_back(character);
                    continue;
                }

                if (!raw.empty() && raw.back() == '\r') {
                    raw.pop_back();
                }
                if (is_utf8(raw) && raw.starts_with('[') &&
                    raw.size() >= timestamp_length + 2) {
                    const auto timestamp_end = raw.find(']');
                    if (timestamp_end != std::string::npos &&
                        timestamp_end == timestamp_length + 1) {
                        const auto timestamp = raw.substr(1, timestamp_length);
                        auto event_part = raw.substr(timestamp_end + 1);
                        while (!event_part.empty() &&
                               (event_part.front() == ' ' ||
                                event_part.front() == '\t')) {
                            event_part.erase(event_part.begin());
                        }
                        Record record;
                        record.timestamp = timestamp;
                        record.component_sort = casefold(component);
                        record.path_sort = casefold(key);
                        record.offset = line_start;
                        record.line = "[" + timestamp + "] [" + component +
                            "] " + event_part;
                        result.records.push_back(std::move(record));
                    }
                }
                raw.clear();
                line_start = position;
            }

            if (!raw.empty() || position > end_offset) {
                crossed = true;
            }
            result.consumed_offset = crossed ? line_start : position;
            return result;
        }

    [[nodiscard]]
    inline MergeResult publish_locked(
        MergeLock& merge_lock,
        const std::filesystem::path& log_directory,
        const std::string_view only_date
    ) {
        // Held across the missing-state check and the publish. WAIT_ABANDONED_0
        // owns the mutex; the journal is inspected before saved offsets are used.
        if (merge_lock.abandoned || journal_exists(log_directory)) {
            if (const auto recovered = recover_interrupted(log_directory);
                !recovered.ok) {
                return recovered;
            }
        }

        const auto components = log_directory / "components";
        std::error_code error;
        if (!std::filesystem::is_directory(components, error) || error) {
            return {};
        }

        const auto state_file = log_directory / "merge.state";
        std::error_code state_error;
        const auto state_presence = regular_file(state_file, state_error);
        if (state_presence == FilePresence::failed) {
            return {false, "Unable to read " + state_file.string()};
        }
        const bool state_existed = state_presence == FilePresence::regular;
        // A missing state file rebuilds every date by replacing the Main log.
        // It does not append from offset zero onto a log that already exists.
        // Limiting that first pass to today would store today's offsets only,
        // and the next full merge would append older dates from offset zero.
        std::string date_filter {only_date};
        if (!state_existed) {
            date_filter.clear();
        }
        auto state = state_existed
            ? load_state(state_file)
            : std::map<std::string, std::uint64_t> {};
        auto logs = discover_logs(log_directory);
        if (!date_filter.empty()) {
            std::erase_if(logs, [&](const SourceLog& log) {
                return log.date != date_filter;
            });
        }

        std::vector<std::string> rebuild_dates;
        auto requires_rebuild = [&rebuild_dates](const std::string& date) {
            return std::ranges::find(rebuild_dates, date) != rebuild_dates.end();
        };
        if (!state_existed) {
            for (const auto& log : logs) {
                if (!requires_rebuild(log.date)) {
                    rebuild_dates.push_back(log.date);
                }
            }
        }
        else {
            for (const auto& log : logs) {
                const auto saved = state.contains(log.key)
                    ? state.at(log.key)
                    : 0;
                std::error_code size_error;
                const auto file_size =
                    std::filesystem::file_size(log.path, size_error);
                if (size_error) {
                    return {
                        false,
                        "Unable to read " + log.path.string()
                    };
                }
                const auto output_file =
                    log_directory / (log.date + "_main.log");
                std::error_code output_error;
                if (file_size < saved ||
                    (saved > 0 &&
                     !std::filesystem::is_regular_file(
                         output_file, output_error
                     ))) {
                    if (!requires_rebuild(log.date)) {
                        rebuild_dates.push_back(log.date);
                    }
                }
            }
        }

        for (const auto& log : logs) {
            if (requires_rebuild(log.date)) {
                state[log.key] = 0;
            }
        }

        // Sizes captured here are the end of this pass. Later appends wait
        // for another merge, which may be the next Auto Core session.
        std::map<std::string, std::uint64_t> snapshots;
        for (const auto& log : logs) {
            std::error_code size_error;
            const auto file_size =
                std::filesystem::file_size(log.path, size_error);
            if (size_error) {
                return {false, "Unable to read " + log.path.string()};
            }
            snapshots.insert_or_assign(log.key, file_size);
        }

        std::map<std::string, std::vector<Record>> records_by_date;
        std::map<std::string, std::uint64_t> pending_offsets;
        for (const auto& log : logs) {
            const auto start_offset = state.contains(log.key)
                ? state.at(log.key)
                : 0;
            const auto end_offset = snapshots.at(log.key);
            if (start_offset >= end_offset) {
                pending_offsets.insert_or_assign(log.key, start_offset);
                continue;
            }
            auto read = read_records(
                log.path,
                log.component,
                log.key,
                start_offset,
                end_offset
            );
            pending_offsets.insert_or_assign(log.key, read.consumed_offset);
            if (!read.records.empty()) {
                auto& bucket = records_by_date[log.date];
                bucket.insert(
                    bucket.end(),
                    std::make_move_iterator(read.records.begin()),
                    std::make_move_iterator(read.records.end())
                );
            }
        }

        try {
            std::vector<JournalOutput> planned;
            for (auto& [date, records] : records_by_date) {
                std::ranges::sort(records, [](const Record& left, const Record& right) {
                    if (left.timestamp != right.timestamp) {
                        return left.timestamp < right.timestamp;
                    }
                    if (left.component_sort != right.component_sort) {
                        return left.component_sort < right.component_sort;
                    }
                    if (left.path_sort != right.path_sort) {
                        return left.path_sort < right.path_sort;
                    }
                    return left.offset < right.offset;
                });

                JournalOutput output;
                output.filename = date + "_main.log";
                output.rebuild = requires_rebuild(date);
                const auto output_file = log_directory / output.filename;
                std::error_code stat_error;
                const auto presence = regular_file(output_file, stat_error);
                if (presence == FilePresence::failed) {
                    return {false, "Unable to read " + output_file.string()};
                }
                output.existed = presence == FilePresence::regular;
                std::uint64_t bytes = 0;
                std::uint64_t hash = fnv_offset;
                for (const auto& record : records) {
                    bytes += record.line.size() + 1;
                    hash = fnv_feed(hash, record.line);
                    hash = fnv_feed(hash, "\n");
                }
                if (output.existed) {
                    output.previous_size = std::filesystem::file_size(
                        output_file,
                        stat_error
                    );
                    if (stat_error) {
                        return {false, "Unable to read " + output_file.string()};
                    }
                    if (output.rebuild &&
                        !fnv_file(output_file, output.hash_before)) {
                        return {false, "Unable to read " + output_file.string()};
                    }
                }
                output.expected_size = output.rebuild
                    ? bytes
                    : output.previous_size + bytes;
                output.hash_after = output.rebuild ? hash : 0;
                for (const auto& log : logs) {
                    if (log.date != date) {
                        continue;
                    }
                    output.offsets.emplace_back(
                        log.key,
                        pending_offsets.at(log.key)
                    );
                }
                planned.push_back(std::move(output));
            }

            const auto journal = journal_path(log_directory);
            if (!planned.empty() && !write_journal(journal, planned)) {
                return {false, "Unable to write " + journal.string()};
            }

            auto write_records = [](
                const std::filesystem::path& path,
                const std::vector<Record>& records,
                const bool rebuild
            ) -> bool {
                std::ofstream output(
                    path,
                    std::ios::binary |
                        (rebuild ? std::ios::trunc : std::ios::app)
                );
                if (!output) {
                    return false;
                }
                for (const auto& record : records) {
                    output << record.line << '\n';
                }
                output.flush();
                return static_cast<bool>(output);
            };

            for (const auto& output : planned) {
                const auto output_file = log_directory / output.filename;
                const auto& records = records_by_date.at(
                    output.filename.substr(0, 10)
                );
                if (output.rebuild) {
                    const auto merging = std::filesystem::path {
                        output_file.wstring() + L".merging"
                    };
                    if (!write_records(merging, records, true)) {
                        return {false, "Unable to write " + merging.string()};
                    }
                }
                else if (!write_records(output_file, records, false)) {
                    return {
                        false,
                        "Unable to write " + output_file.string()
                    };
                }
            }

            for (const auto& output : planned) {
                if (!output.rebuild) {
                    continue;
                }
                const auto output_file = log_directory / output.filename;
                const auto merging = std::filesystem::path {
                    output_file.wstring() + L".merging"
                };
                if (MoveFileExW(
                        merging.c_str(),
                        output_file.c_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
                    ) == 0) {
                    return {
                        false,
                        "Unable to write " + output_file.string()
                    };
                }
            }

            for (const auto& output : planned) {
                if (output.rebuild) {
                    continue;
                }
                const auto output_file = log_directory / output.filename;
                std::error_code size_error;
                const auto size = std::filesystem::file_size(
                    output_file,
                    size_error
                );
                if (size_error || size != output.expected_size) {
                    return {
                        false,
                        "Unable to write " + output_file.string()
                    };
                }
            }

            for (const auto& [key, offset] : pending_offsets) {
                state.insert_or_assign(key, offset);
            }
            if (!save_state(state_file, state)) {
                return {false, "Unable to write " + state_file.string()};
            }
            if (!planned.empty()) {
                remove_regular(journal);
            }
        }
        catch (const std::exception& failure) {
            return {false, failure.what()};
        }
        return {};
    }

    inline MergeResult merge_once(
        const std::filesystem::path& log_directory,
        const std::string_view only_date = {}
    ) {
        auto acquired = acquire_merge_lock(nullptr);
        if (acquired.kind != AcquireKind::owned) {
            return {false, "Unable to lock the log merge."};
        }
        return publish_locked(acquired.lock, log_directory, only_date);
    }

    /**
     * Waits for the merge mutex unless `stop_event` is signaled first.
     * A pass that has acquired the mutex runs to completion.
     */
    inline MergeResult merge_once_interruptible(
        const std::filesystem::path& log_directory,
        const HANDLE stop_event,
        const std::string_view only_date = {}
    ) {
        auto acquired = acquire_merge_lock(stop_event);
        if (acquired.kind == AcquireKind::stopped) {
            MergeResult stopped;
            stopped.stopped = true;
            return stopped;
        }
        if (acquired.kind != AcquireKind::owned) {
            return {false, "Unable to lock the log merge."};
        }
        return publish_locked(acquired.lock, log_directory, only_date);
    }

} // namespace ac::logger::detail
