module;

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "component_log_format_detail.hpp"

module auto_core.core.component;

import :logger;
import std;
import auto_core.core.clock;
import auto_core.core.error;

namespace {

    constexpr std::wstring_view mutex_prefix = L"Local\\AutoCore.component.log.";
    constexpr std::uint64_t fnv_offset = 14695981039346656037ull;
    constexpr std::uint64_t fnv_prime = 1099511628211ull;

    std::wstring mutex_name_for(const std::filesystem::path& path) {
        std::error_code error;
        auto absolute = std::filesystem::absolute(path, error);
        if (error) {
            absolute = path;
        }
        auto key = absolute.lexically_normal().wstring();
        for (wchar_t& character : key) {
            if (character == L'\\') {
                character = L'/';
            }
        }

        std::wstring name;
        name.reserve(mutex_prefix.size() + key.size());
        name.append(mutex_prefix);
        name.append(key);
        if (name.size() < MAX_PATH) {
            return name;
        }

        std::uint64_t hash = fnv_offset;
        for (const wchar_t character : key) {
            const auto value = static_cast<std::uint32_t>(character);
            hash ^= value & 0xFFu;
            hash *= fnv_prime;
            hash ^= (value >> 8) & 0xFFu;
            hash *= fnv_prime;
        }
        constexpr wchar_t digits[] = L"0123456789abcdef";
        wchar_t hex[16];
        for (int index = 15; index >= 0; --index) {
            hex[index] = digits[hash & 0xF];
            hash >>= 4;
        }
        name.assign(mutex_prefix);
        name.append(hex, hex + 16);
        return name;
    }

    std::string win32_message(const DWORD code) {
        return std::system_category().message(static_cast<int>(code));
    }

    void finish_open_line(
        std::string& pending,
        bool& line_open,
        std::vector<std::string>& completed
    ) {
        if (!line_open) {
            return;
        }
        pending.push_back('\n');
        completed.push_back(std::move(pending));
        pending.clear();
        line_open = false;
    }

    /**
     * Formats the same records as `write_log_record`, but keeps an unfinished
     * line in memory. Each completed entry in `completed` ends with one `\n`
     * and contains no earlier newline.
     */
    void collect_log_lines(
        std::string& pending,
        bool& line_open,
        std::vector<std::string>& completed,
        const std::string_view timestamp,
        const std::string_view event_type,
        const std::string_view message,
        const bool newline,
        const std::string_view area
    ) {
        const auto start_line = [&] {
            pending.push_back('[');
            pending.append(timestamp);
            pending.append("] ");
            if (!area.empty()) {
                pending.push_back('[');
                pending.append(area);
                pending.append("] ");
            }
            if (!event_type.empty()) {
                pending.push_back('[');
                pending.append(event_type);
                pending.append("] ");
            }
            line_open = true;
        };

        const auto finish_line = [&] {
            pending.push_back('\n');
            completed.push_back(std::move(pending));
            pending.clear();
            line_open = false;
        };

        std::size_t offset = 0;
        while (offset < message.size()) {
            if (!line_open) {
                start_line();
            }
            const std::size_t end = message.find('\n', offset);
            if (end == std::string_view::npos) {
                pending.append(message.substr(offset));
                offset = message.size();
            }
            else {
                pending.append(message.substr(offset, end - offset));
                finish_line();
                offset = end + 1;
            }
        }

        if (message.empty() && !line_open) {
            start_line();
        }
        if (newline && line_open) {
            finish_line();
        }
    }

} // namespace

namespace ac::component_detail {

    class ComponentLogger::Impl {
    public:
        Impl(
            const std::string_view component_name,
            std::filesystem::path log_directory,
            const ac::clock::DateTime& component_session_start,
            const std::string_view log_file_name
        )
            : session_start(component_session_start),
              name(component_name),
              file_name(
                  log_file_name.empty()
                      ? std::string {component_name}
                      : std::string {log_file_name}
              ),
              directory(std::move(log_directory)),
              area(file_name == name ? std::string {} : name) {
        }

        ~Impl() {
            close_handle();
        }

        ac::clock::DateTime session_start;
        std::string name;
        std::string file_name;
        std::filesystem::path directory;
        std::string area;
        std::string logger_date;
        std::string pending;
        bool line_open = false;
        HANDLE file = INVALID_HANDLE_VALUE;
        HANDLE file_mutex = nullptr;
        std::filesystem::path open_path;
        std::mutex mutex;

        std::filesystem::path log_path(const std::string_view date) const {
            return directory / (std::string {date} + "_" + file_name + ".log");
        }

        void close_handle() noexcept {
            if (file != INVALID_HANDLE_VALUE) {
                CloseHandle(file);
                file = INVALID_HANDLE_VALUE;
            }
            if (file_mutex != nullptr) {
                CloseHandle(file_mutex);
                file_mutex = nullptr;
            }
            open_path.clear();
        }

        [[nodiscard]]
        bool ensure_open(const std::filesystem::path& path) {
            if (file != INVALID_HANDLE_VALUE && open_path == path) {
                return true;
            }
            close_handle();

            std::error_code error;
            std::filesystem::create_directories(directory, error);
            if (error) {
                ac::error::log(
                    "Failed to create component log directory: {} - {}",
                    directory,
                    error.message()
                );
                return false;
            }

            file = CreateFileW(
                path.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );
            if (file == INVALID_HANDLE_VALUE) {
                ac::error::log(
                    "Failed to open component log file: {} - {}",
                    path,
                    win32_message(GetLastError())
                );
                return false;
            }

            const auto mutex_name = mutex_name_for(path);
            file_mutex = CreateMutexW(nullptr, FALSE, mutex_name.c_str());
            if (file_mutex == nullptr) {
                ac::error::log(
                    "Failed to open component log mutex: {} - {}",
                    path,
                    win32_message(GetLastError())
                );
                close_handle();
                return false;
            }

            open_path = path;
            return true;
        }

        [[nodiscard]]
        bool file_size(LARGE_INTEGER& size) const {
            return GetFileSizeEx(file, &size) != FALSE;
        }

        [[nodiscard]]
        bool seek_to(const LARGE_INTEGER position) const {
            return SetFilePointerEx(file, position, nullptr, FILE_BEGIN) !=
                FALSE;
        }

        [[nodiscard]]
        bool ends_with_newline() const {
            LARGE_INTEGER size {};
            if (!file_size(size) || size.QuadPart <= 0) {
                return true;
            }
            LARGE_INTEGER last = size;
            last.QuadPart -= 1;
            if (!seek_to(last)) {
                return false;
            }
            char byte = '\0';
            DWORD read = 0;
            if (ReadFile(file, &byte, 1, &read, nullptr) == FALSE || read != 1) {
                return false;
            }
            return byte == '\n';
        }

        /**
         * Turns a fragment that has no newline into its own line so the next
         * record cannot be parsed as a continuation of it.
         */
        [[nodiscard]]
        bool seal_partial_tail() {
            if (ends_with_newline()) {
                return true;
            }
            LARGE_INTEGER size {};
            if (!file_size(size) || !seek_to(size)) {
                return false;
            }
            const char newline = '\n';
            DWORD wrote = 0;
            return WriteFile(file, &newline, 1, &wrote, nullptr) != FALSE &&
                wrote == 1;
        }

        [[nodiscard]]
        bool truncate_to(const LARGE_INTEGER size) {
            return seek_to(size) && SetEndOfFile(file) != FALSE;
        }

        /**
         * Writes one complete line. A short or failed write is removed before
         * the file mutex is released. If the extra bytes cannot be removed,
         * they are closed with a newline so a later record stays a separate line.
         */
        [[nodiscard]]
        bool append_line(const std::string_view line) {
            if (line.empty() || line.size() > std::numeric_limits<DWORD>::max()) {
                ac::error::log(
                    "Failed to append component log record: {}",
                    open_path
                );
                return false;
            }

            LARGE_INTEGER original {};
            if (!file_size(original)) {
                ac::error::log(
                    "Failed to size component log file: {} - {}",
                    open_path,
                    win32_message(GetLastError())
                );
                return false;
            }

            const auto* data = line.data();
            const auto size = static_cast<DWORD>(line.size());
            DWORD written = 0;
            DWORD write_error = ERROR_SUCCESS;
            bool failed = false;
            if (!seek_to(original)) {
                failed = true;
                write_error = GetLastError();
            }
            while (!failed && written < size) {
                DWORD wrote = 0;
                const BOOL ok = WriteFile(
                    file,
                    data + written,
                    size - written,
                    &wrote,
                    nullptr
                );
                if (wrote > 0) {
                    written += wrote;
                }
                if (written >= size) {
                    break;
                }
                if (ok == FALSE || wrote == 0) {
                    failed = true;
                    write_error = GetLastError();
                }
            }

            if (!failed && written == size) {
                return true;
            }

            // The newline is the last byte, so a short write has no record
            // boundary yet. Remove those bytes before releasing the file
            // mutex. A later writer must not append a valid line onto them:
            // the merger would treat the fragment and that line as one record.
            if (written > 0 && !truncate_to(original) && !seal_partial_tail()) {
                ac::error::log(
                    "Failed to seal partial component log record: {}",
                    open_path
                );
            }
            ac::error::log(
                "Failed to append component log record: {} - {}",
                open_path,
                win32_message(write_error)
            );
            return false;
        }

        [[nodiscard]]
        bool append_completed(const std::vector<std::string>& lines) {
            if (lines.empty()) {
                return true;
            }
            if (file == INVALID_HANDLE_VALUE || file_mutex == nullptr) {
                return false;
            }

            const DWORD waited = WaitForSingleObject(file_mutex, INFINITE);
            if (waited != WAIT_OBJECT_0 && waited != WAIT_ABANDONED_0) {
                ac::error::log(
                    "Failed to lock component log: {} - {}",
                    open_path,
                    win32_message(GetLastError())
                );
                return false;
            }
            struct Release {
                HANDLE mutex;
                ~Release() {
                    ReleaseMutex(mutex);
                }
            } release {file_mutex};

            // A crashed or short write can leave a tail with no newline.
            // Close that tail before adding a record. Otherwise the merger
            // reads the tail and the new line as one record and can skip both.
            if (!ends_with_newline() && !seal_partial_tail()) {
                ac::error::log(
                    "Failed to seal partial component log record: {}",
                    open_path
                );
                return false;
            }

            for (const auto& line : lines) {
                if (!append_line(line)) {
                    if (FlushFileBuffers(file) == FALSE) {
                        ac::error::log(
                            "Failed to flush component log: {}",
                            open_path
                        );
                    }
                    return false;
                }
            }
            if (FlushFileBuffers(file) == FALSE) {
                ac::error::log(
                    "Failed to flush component log: {}",
                    open_path
                );
            }
            return true;
        }

        void emit_marker(
            const std::string_view timestamp,
            const std::string_view marker
        ) {
            std::vector<std::string> completed;
            finish_open_line(pending, line_open, completed);
            collect_log_lines(
                pending,
                line_open,
                completed,
                timestamp,
                {},
                marker,
                true,
                area
            );
            (void)append_completed(completed);
        }

        void open_files_unlocked(
            const std::string_view date,
            const std::string_view timestamp,
            const bool initial
        ) {
            if (!ensure_open(log_path(date))) {
                return;
            }
            logger_date = date;

            const std::string marker = initial
                ? std::format(
                    "Session started for {} {}",
                    name,
                    ac::clock::format_datetime(session_start)
                )
                : std::format(
                    "--- Session continues from {} ---",
                    ac::clock::format_datetime(session_start)
                );
            emit_marker(timestamp, marker);
        }

        void update_files_unlocked(
            const std::string_view date,
            const std::string_view timestamp
        ) {
            if (logger_date == date && file != INVALID_HANDLE_VALUE) {
                return;
            }

            if (file != INVALID_HANDLE_VALUE) {
                emit_marker(
                    timestamp,
                    "--- Session continues in next log file ---"
                );
                close_handle();
                logger_date.clear();
            }

            open_files_unlocked(date, timestamp, false);
        }

        void close_files_unlocked() {
            if (file == INVALID_HANDLE_VALUE) {
                pending.clear();
                line_open = false;
                return;
            }

            const auto end = ac::clock::get_local_datetime();
            const std::string timestamp = ac::clock::format_log_timestamp(end);
            emit_marker(
                timestamp,
                std::format(
                    "Session ended for {} {} at {}",
                    name,
                    end.date_iso,
                    end.timestamp_with_seconds
                )
            );
            close_handle();
            logger_date.clear();
        }
    };

    ComponentLogger::ComponentLogger(
        const std::string_view component_name,
        std::filesystem::path directory,
        const ac::clock::DateTime& session_start,
        const std::string_view file_name
    )
        : impl_(std::make_unique<Impl>(
            component_name,
            std::move(directory),
            session_start,
            file_name
        )) {
        std::scoped_lock lock(impl_->mutex);
        impl_->open_files_unlocked(
            session_start.date_iso,
            ac::clock::format_log_timestamp(session_start),
            true
        );
    }

    ComponentLogger::~ComponentLogger() noexcept {
        try {
            std::scoped_lock lock(impl_->mutex);
            impl_->close_files_unlocked();
        }
        catch (const std::exception& exception) {
            ac::error::log(
                "Unable to close component logs: {}",
                exception.what()
            );
        }
        catch (...) {
            ac::error::log(
                "Unable to close component logs: unknown error."
            );
        }
    }

    void ComponentLogger::write(
        const std::string_view timestamp,
        const std::string_view date_iso,
        const std::string_view event_type,
        const std::string_view message,
        const bool newline
    ) {
        std::scoped_lock lock(impl_->mutex);
        impl_->update_files_unlocked(date_iso, timestamp);
        if (impl_->file == INVALID_HANDLE_VALUE) {
            return;
        }

        std::vector<std::string> completed;
        collect_log_lines(
            impl_->pending,
            impl_->line_open,
            completed,
            timestamp,
            event_type,
            message,
            newline,
            impl_->area
        );
        (void)impl_->append_completed(completed);
    }

    void ComponentLogger::update_file() {
        const auto now = ac::clock::get_local_datetime();
        std::scoped_lock lock(impl_->mutex);
        impl_->update_files_unlocked(
            now.date_iso,
            ac::clock::format_log_timestamp(now)
        );
    }

    void ComponentLogger::flush() {
        std::scoped_lock lock(impl_->mutex);
        if (impl_->file != INVALID_HANDLE_VALUE) {
            if (FlushFileBuffers(impl_->file) == FALSE) {
                ac::error::log(
                    "Failed to flush component log: {}",
                    impl_->open_path
                );
            }
        }
    }

} // namespace ac::component_detail
