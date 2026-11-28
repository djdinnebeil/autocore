module;

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

module auto_core.main.log_merge_worker;

import std;
import auto_core.core.logging.config;
import auto_core.utility.log_merge;

struct LogMergeWorker::State {
    HANDLE stop = nullptr;
    std::jthread thread;
};

namespace {

    void run_periodic(const HANDLE stop) {
        const auto interval = ac::log_merge::merge_interval_seconds();
        const auto directory = ac::logging::config::directory();
        try {
            if (interval == 0) {
                if (stop != nullptr) {
                    WaitForSingleObject(stop, INFINITE);
                }
                return;
            }

            const auto interval_ms = interval > 0xFFFFFFFEu / 1000
                ? DWORD {0xFFFFFFFEu}
                : static_cast<DWORD>(interval * 1000);
            for (;;) {
                const auto result = ac::log_merge::merge_once_interruptible(
                    directory,
                    stop
                );
                if (result.stopped) {
                    return;
                }
                ac::log_merge::report(result);
                if (result.ok) {
                    ac::log_merge::log_periodic(interval);
                }
                if (stop == nullptr) {
                    return;
                }
                const DWORD wait = WaitForSingleObject(stop, interval_ms);
                if (wait != WAIT_TIMEOUT) {
                    return;
                }
            }
        }
        catch (const std::exception& exception) {
            ac::log_merge::report({
                .ok = false,
                .message = std::string {"Log merge failed: "} + exception.what()
            });
        }
        catch (...) {
            ac::log_merge::report({
                .ok = false,
                .message = "Log merge failed."
            });
        }
    }

} // namespace

LogMergeWorker::LogMergeWorker() noexcept = default;

LogMergeWorker::~LogMergeWorker() {
    request_stop();
    join();
}

LogMergeWorker::LogMergeWorker(LogMergeWorker&&) noexcept = default;

LogMergeWorker& LogMergeWorker::operator=(LogMergeWorker&&) noexcept = default;

void LogMergeWorker::request_stop() noexcept {
    if (state_ == nullptr || state_->stop == nullptr) {
        return;
    }
    SetEvent(state_->stop);
}

void LogMergeWorker::join() {
    if (state_ == nullptr) {
        return;
    }
    if (state_->thread.joinable()) {
        state_->thread.join();
    }
    if (state_->stop != nullptr) {
        CloseHandle(state_->stop);
        state_->stop = nullptr;
    }
}

LogMergeWorker start_periodic_merge(const bool automatic) {
    LogMergeWorker worker;
    if (!automatic) {
        return worker;
    }
    worker.state_ = std::make_unique<LogMergeWorker::State>();
    worker.state_->stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    const HANDLE stop = worker.state_->stop;
    worker.state_->thread = std::jthread {[stop] {
        run_periodic(stop);
    }};
    return worker;
}
