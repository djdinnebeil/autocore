/**
 * \file log_merge_worker.ixx
 * \brief Periodic log merge inside auto_core.exe.
 *
 * Interval `0` waits for shutdown and does not merge. A positive interval
 * merges once, then again after each interval. Shutdown can interrupt the
 * wait for the merge mutex. An active merge finishes. When automatic merging
 * is off, `start_periodic_merge` returns an idle worker with no thread.
 * `request_stop` and `join` are safe on that worker.
 */
export module auto_core.main.log_merge_worker;

import std;

export class LogMergeWorker {
public:
    LogMergeWorker() noexcept;
    ~LogMergeWorker();
    LogMergeWorker(LogMergeWorker&&) noexcept;
    LogMergeWorker& operator=(LogMergeWorker&&) noexcept;
    LogMergeWorker(const LogMergeWorker&) = delete;
    LogMergeWorker& operator=(const LogMergeWorker&) = delete;

    void request_stop() noexcept;
    void join();

    friend LogMergeWorker start_periodic_merge(bool automatic);

private:
    struct State;
    std::unique_ptr<State> state_;
};

export [[nodiscard]] LogMergeWorker start_periodic_merge(bool automatic);
