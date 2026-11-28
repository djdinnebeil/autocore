/**
 * \file logging_cleanup_worker.ixx
 * \brief Background log cleanup for auto_core.exe.
 *
 * The first pass runs immediately. Later passes wait until the next local
 * midnight. `request_stop` ends the wait and lets an in-flight pass finish.
 */
export module auto_core.main.logging_cleanup_worker;

import std;

export class LoggingCleanupWorker {
public:
    LoggingCleanupWorker() noexcept;
    ~LoggingCleanupWorker();
    LoggingCleanupWorker(LoggingCleanupWorker&&) noexcept;
    LoggingCleanupWorker& operator=(LoggingCleanupWorker&&) noexcept;
    LoggingCleanupWorker(const LoggingCleanupWorker&) = delete;
    LoggingCleanupWorker& operator=(const LoggingCleanupWorker&) = delete;

    void request_stop() noexcept;
    void join();

    friend LoggingCleanupWorker start_logging_cleanup();

private:
    struct State;
    std::unique_ptr<State> state_;
};

export [[nodiscard]] LoggingCleanupWorker start_logging_cleanup();
