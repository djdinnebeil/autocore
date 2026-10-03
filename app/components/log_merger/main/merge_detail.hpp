/**
 * \file merge_detail.hpp
 * \brief Incremental merge of component `.main.log` files.
 *
 * Behavior follows `dist/logs/merge_incremental.py`. Callers own scheduling.
 * This header does not read pipes or `logger.ini`.
 */
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ac::logger::detail {

    struct MergeResult {
        bool ok = true;
        std::string message;
    };

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
        bool is_main_log_name(
            const std::string& name,
            std::string& date
        ) {
            constexpr std::string_view suffix = ".main.log";
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
                std::string date;
                if (!is_main_log_name(entry.path().filename().string(), date)) {
                    continue;
                }
                SourceLog log;
                log.path = entry.path();
                log.date = std::move(date);
                log.component = entry.path().parent_path().filename().string();
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
    inline MergeResult merge_once(const std::filesystem::path& log_directory) {
        const auto components = log_directory / "components";
        std::error_code error;
        if (!std::filesystem::is_directory(components, error)) {
            return {};
        }

        const auto state_file = log_directory / "merge.state";
        const bool state_existed =
            std::filesystem::is_regular_file(state_file, error);
        auto state = state_existed
            ? load_state(state_file)
            : std::map<std::string, std::uint64_t> {};
        const auto logs = discover_logs(log_directory);

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

                const auto output_file = log_directory / (date + "_main.log");
                const auto mode = requires_rebuild(date)
                    ? std::ios::trunc
                    : std::ios::app;
                std::ofstream output(
                    output_file,
                    std::ios::binary | mode
                );
                if (!output) {
                    return {
                        false,
                        "Unable to write " + output_file.string()
                    };
                }
                for (const auto& record : records) {
                    output << record.line << '\n';
                }
                if (!output) {
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
        }
        catch (const std::exception& failure) {
            return {false, failure.what()};
        }
        return {};
    }

} // namespace ac::logger::detail
