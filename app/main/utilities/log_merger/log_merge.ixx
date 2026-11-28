/**
 * \file log_merge.ixx
 * \brief Shared log merge for auto_core.exe and auto_core_logger.exe.
 *
 * Scheduling stays with the caller. `merge_once` blocks on the merge mutex.
 * `merge_once_interruptible` returns when the stop event is signaled before
 * that mutex is acquired. A pass that already holds the mutex finishes.
 */
export module auto_core.utility.log_merge;

import std;

export namespace ac::log_merge {

    struct Result {
        bool ok = true;
        std::string message;
        bool stopped = false;
    };

    [[nodiscard]] std::uint64_t merge_interval_seconds();
    [[nodiscard]] bool merge_logs_on_shutdown();

    [[nodiscard]] Result merge_once(
        const std::filesystem::path& log_directory,
        std::string_view only_date = {}
    );

    [[nodiscard]] Result merge_once_interruptible(
        const std::filesystem::path& log_directory,
        void* stop_event,
        std::string_view only_date = {}
    );

    void report(const Result& result);
    void log_refresh_success();
    void log_periodic(std::uint64_t interval_seconds);
    void log_oneshot(bool shutdown_launch);

} // namespace ac::log_merge
