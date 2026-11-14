module;

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

module auto_core.utility.logging_cleanup;

import std;

import auto_core.core.clock;
import auto_core.core.logging.config;

namespace {

    constexpr std::int64_t bytes_per_megabyte = 1048576;
    constexpr wchar_t mutex_name[] = L"Local\\AutoCore.logging_cleanup";

    struct LogFile {
        std::filesystem::path path;
        std::filesystem::file_time_type written {};
        std::uint64_t size = 0;
        bool current_day = false;
        bool gone = false;
    };

    class MutexLock {
    public:
        explicit MutexLock(const HANDLE handle) noexcept
            : handle_(handle) {
        }

        ~MutexLock() {
            if (handle_ != nullptr) {
                ::ReleaseMutex(handle_);
                ::CloseHandle(handle_);
            }
        }

        MutexLock(const MutexLock&) = delete;
        MutexLock& operator=(const MutexLock&) = delete;

    private:
        HANDLE handle_ = nullptr;
    };

    enum class DeleteResult {
        deleted,
        missing,
        failed
    };

    void append_detail(std::string& detail, const std::string& line) {
        if (line.empty()) {
            return;
        }
        if (!detail.empty()) {
            detail.push_back('\n');
        }
        detail += line;
    }

    [[nodiscard]] bool has_log_extension(const std::filesystem::path& path) {
        const auto extension = path.extension().wstring();
        return extension.size() == 4 &&
            extension[0] == L'.' &&
            (extension[1] == L'l' || extension[1] == L'L') &&
            (extension[2] == L'o' || extension[2] == L'O') &&
            (extension[3] == L'g' || extension[3] == L'G');
    }

    /**
     * Today's logs are `{local date}_{name}.log`, including `{date}_main.log`.
     * The date is the filename prefix, not the file's last write time.
     */
    [[nodiscard]] bool is_current_day_log(
        const std::filesystem::path& path,
        const std::string_view today
    ) {
        const auto name = path.filename().string();
        return name.size() > today.size() &&
            name[today.size()] == '_' &&
            name.starts_with(today);
    }

    [[nodiscard]] bool is_directory_reparse_point(
        const std::filesystem::path& path
    ) {
        const DWORD attributes = ::GetFileAttributesW(path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    [[nodiscard]] std::filesystem::file_time_type retention_cutoff(
        const std::int64_t days
    ) {
        using clock = std::filesystem::file_time_type::clock;
        const auto now = clock::now();
        if (days <= 0) {
            return now;
        }

        using rep = clock::duration::rep;
        const auto day_ticks = std::chrono::duration_cast<clock::duration>(
            std::chrono::seconds {86400}
        ).count();
        if (day_ticks <= 0) {
            return now;
        }

        const auto max_days = clock::duration::max().count() / day_ticks;
        if (days >= max_days) {
            return clock::time_point::min();
        }
        return now - clock::duration {day_ticks * static_cast<rep>(days)};
    }

    [[nodiscard]] std::uint64_t limit_bytes(const std::int64_t megabytes) {
        if (megabytes <= 0) {
            return 0;
        }
        constexpr auto max_megabytes =
            std::numeric_limits<std::uint64_t>::max() /
            static_cast<std::uint64_t>(bytes_per_megabyte);
        if (static_cast<std::uint64_t>(megabytes) > max_megabytes) {
            return std::numeric_limits<std::uint64_t>::max();
        }
        return static_cast<std::uint64_t>(megabytes) *
            static_cast<std::uint64_t>(bytes_per_megabyte);
    }

    [[nodiscard]] int compare_paths(
        const std::filesystem::path& left,
        const std::filesystem::path& right
    ) {
        const int result = ::CompareStringOrdinal(
            left.c_str(),
            -1,
            right.c_str(),
            -1,
            TRUE
        );
        if (result == CSTR_LESS_THAN) {
            return -1;
        }
        if (result == CSTR_GREATER_THAN) {
            return 1;
        }
        return 0;
    }

    void add_size(std::uint64_t& total, const std::uint64_t size) {
        if (total > std::numeric_limits<std::uint64_t>::max() - size) {
            total = std::numeric_limits<std::uint64_t>::max();
            return;
        }
        total += size;
    }

    void subtract_size(std::uint64_t& total, const std::uint64_t size) {
        if (total >= size) {
            total -= size;
            return;
        }
        total = 0;
    }

    [[nodiscard]] DeleteResult delete_log(
        const std::filesystem::path& path,
        std::string& detail
    ) {
        std::error_code error;
        const bool removed = std::filesystem::remove(path, error);
        if (!error && removed) {
            return DeleteResult::deleted;
        }
        if (!error || error == std::errc::no_such_file_or_directory) {
            return DeleteResult::missing;
        }
        append_detail(
            detail,
            "Could not delete '" + path.string() + "': " + error.message()
        );
        return DeleteResult::failed;
    }

    [[nodiscard]] ac::logging_cleanup::Result make_result(
        const ac::logging_cleanup::Status status,
        std::string detail,
        ac::logging_cleanup::Summary summary = {}
    ) {
        return {
            .status = status,
            .detail = std::move(detail),
            .summary = std::move(summary)
        };
    }

    [[nodiscard]] std::vector<LogFile> collect_logs(
        const std::filesystem::path& root,
        const std::string_view today
    ) {
        std::vector<LogFile> files;
        std::filesystem::recursive_directory_iterator iterator {root};
        const std::filesystem::recursive_directory_iterator end;
        for (; iterator != end; ++iterator) {
            const auto& entry = *iterator;
            if (is_directory_reparse_point(entry.path())) {
                iterator.disable_recursion_pending();
                continue;
            }

            std::error_code error;
            if (entry.is_directory(error) && !error) {
                continue;
            }

            error.clear();
            if (!entry.is_regular_file(error) || error) {
                continue;
            }
            if (!has_log_extension(entry.path())) {
                continue;
            }

            error.clear();
            const auto size = entry.file_size(error);
            if (error) {
                continue;
            }
            error.clear();
            const auto written = entry.last_write_time(error);
            if (error) {
                continue;
            }

            files.push_back(LogFile {
                .path = entry.path(),
                .written = written,
                .size = static_cast<std::uint64_t>(size),
                .current_day = is_current_day_log(entry.path(), today)
            });
        }
        return files;
    }

} // namespace

ac::logging_cleanup::Result ac::logging_cleanup::run(const Request& request) {
    const HANDLE mutex = ::CreateMutexW(nullptr, FALSE, mutex_name);
    if (mutex == nullptr) {
        return make_result(Status::failed, "Unable to create the log cleanup lock");
    }
    const DWORD acquired = ::WaitForSingleObject(mutex, 0);
    if (acquired == WAIT_TIMEOUT) {
        ::CloseHandle(mutex);
        return make_result(Status::busy, "Log cleanup is already running");
    }
    if (acquired != WAIT_OBJECT_0 && acquired != WAIT_ABANDONED) {
        ::CloseHandle(mutex);
        return make_result(Status::failed, "Unable to acquire the log cleanup lock");
    }
    const MutexLock lock {mutex};

    ac::logging::config::resolve();
    const auto& directory = ac::logging::config::directory();
    std::error_code error;
    const bool exists = std::filesystem::exists(directory, error);
    if (error || !exists) {
        return make_result(
            Status::failed,
            "Log directory does not exist: " + directory.string()
        );
    }
    error.clear();
    if (!std::filesystem::is_directory(directory, error) || error) {
        return make_result(
            Status::failed,
            "Log path is not a directory: " + directory.string()
        );
    }

    const auto today = ac::clock::get_date_iso();
    std::vector<LogFile> files;
    try {
        files = collect_logs(directory, today);
    }
    catch (const std::filesystem::filesystem_error& failure) {
        return make_result(
            Status::failed,
            std::string {"Could not read log directory: "} + failure.what()
        );
    }

    const bool dry_run = request.dry_run;
    const auto cutoff = retention_cutoff(ac::logging::config::retention_days());
    int retention_deleted = 0;
    int projected_retention = 0;
    bool failed = false;
    std::string detail;
    for (auto& file : files) {
        if (file.current_day || !(file.written < cutoff)) {
            continue;
        }
        if (dry_run) {
            file.gone = true;
            ++projected_retention;
            continue;
        }

        const auto deleted = delete_log(file.path, detail);
        if (deleted == DeleteResult::deleted) {
            file.gone = true;
            ++retention_deleted;
            continue;
        }
        if (deleted == DeleteResult::missing) {
            file.gone = true;
            continue;
        }
        failed = true;
        break;
    }

    std::vector<LogFile> survivors;
    survivors.reserve(files.size());
    for (auto& file : files) {
        if (!file.gone) {
            survivors.push_back(std::move(file));
        }
    }
    std::uint64_t remaining = 0;
    bool protected_logs = false;
    for (const auto& file : survivors) {
        add_size(remaining, file.size);
        protected_logs = protected_logs || file.current_day;
    }

    const auto limit = limit_bytes(ac::logging::config::max_directory_mb());
    int size_deleted = 0;
    int projected_size = 0;
    bool eligible_remains = false;
    if (remaining > limit) {
        std::ranges::sort(survivors, [](const LogFile& left, const LogFile& right) {
            if (left.written != right.written) {
                return left.written < right.written;
            }
            return compare_paths(left.path, right.path) < 0;
        });
    }
    for (const auto& file : survivors) {
        if (remaining <= limit) {
            break;
        }
        if (file.current_day) {
            continue;
        }
        if (dry_run) {
            subtract_size(remaining, file.size);
            ++projected_size;
            continue;
        }

        const auto deleted = delete_log(file.path, detail);
        if (deleted == DeleteResult::deleted) {
            subtract_size(remaining, file.size);
            ++size_deleted;
            continue;
        }
        if (deleted == DeleteResult::missing) {
            subtract_size(remaining, file.size);
            continue;
        }
        failed = true;
        eligible_remains = true;
    }

    if (!dry_run) {
        try {
            std::uint64_t rescanned = 0;
            for (const auto& file : collect_logs(directory, today)) {
                add_size(rescanned, file.size);
            }
            remaining = rescanned;
        }
        catch (const std::filesystem::filesystem_error& failure) {
            append_detail(
                detail,
                std::string {"Could not rescan log directory: "} + failure.what()
            );
            failed = true;
        }
    }

    Summary summary {
        .available = true,
        .dry_run = dry_run,
        .retention_deleted = retention_deleted,
        .size_deleted = size_deleted,
        .projected_retention = projected_retention,
        .projected_size = projected_size,
        .blocked_by_today =
            remaining > limit && protected_logs && !eligible_remains,
        .remaining_bytes = remaining
    };
    if (!failed) {
        return make_result(Status::finished, {}, std::move(summary));
    }
    if (detail.empty()) {
        detail = "Log cleanup failed.";
    }
    return make_result(Status::incomplete, std::move(detail), std::move(summary));
}
