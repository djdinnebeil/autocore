module;

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

module auto_core.main.logging_cleanup_worker;

import std;
import auto_core.main.application;
import auto_core.utility.logging_cleanup;

struct LoggingCleanupWorker::State {
    HANDLE stop = nullptr;
    std::jthread thread;
};

namespace {

    [[nodiscard]]
    DWORD milliseconds_until_next_local_midnight() {
        constexpr DWORD fallback = 24u * 60u * 60u * 1000u;
        SYSTEMTIME local {};
        GetLocalTime(&local);
        local.wHour = 0;
        local.wMinute = 0;
        local.wSecond = 0;
        local.wMilliseconds = 0;

        constexpr int days_in_month[] {
            0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };
        if (local.wMonth < 1 || local.wMonth > 12) {
            return fallback;
        }
        const bool leap =
            (local.wYear % 4 == 0 && local.wYear % 100 != 0) ||
            (local.wYear % 400 == 0);
        int length = days_in_month[local.wMonth];
        if (local.wMonth == 2 && leap) {
            length = 29;
        }
        if (local.wDay < length) {
            ++local.wDay;
        }
        else if (local.wMonth < 12) {
            local.wDay = 1;
            ++local.wMonth;
        }
        else {
            local.wDay = 1;
            local.wMonth = 1;
            ++local.wYear;
        }

        SYSTEMTIME utc {};
        if (!TzSpecificLocalTimeToSystemTime(nullptr, &local, &utc)) {
            return fallback;
        }
        FILETIME next_file_time {};
        if (!SystemTimeToFileTime(&utc, &next_file_time)) {
            return fallback;
        }
        FILETIME now_file_time {};
        GetSystemTimeAsFileTime(&now_file_time);

        ULARGE_INTEGER next {};
        next.LowPart = next_file_time.dwLowDateTime;
        next.HighPart = next_file_time.dwHighDateTime;
        ULARGE_INTEGER now {};
        now.LowPart = now_file_time.dwLowDateTime;
        now.HighPart = now_file_time.dwHighDateTime;
        if (next.QuadPart <= now.QuadPart) {
            return 1000;
        }
        const auto milliseconds =
            (next.QuadPart - now.QuadPart) / 10000ULL;
        if (milliseconds == 0) {
            return 1;
        }
        if (milliseconds > fallback) {
            return fallback;
        }
        return static_cast<DWORD>(milliseconds);
    }

    void log_cleanup_summary(const ac::logging_cleanup::Summary& summary) {
        if (!summary.available) {
            return;
        }
        const int total = summary.retention_deleted + summary.size_deleted;
        if (summary.dry_run) {
            auto_core.log_debug(
                "Dry run. Log cleanup deleted {} file(s) by retention, {} by size, {} total. Projected retention deletions: {}. Projected size deletions: {}.",
                summary.retention_deleted,
                summary.size_deleted,
                total,
                summary.projected_retention,
                summary.projected_size
            );
        }
        else {
            auto_core.log_debug(
                "Log cleanup deleted {} file(s) by retention, {} by size, {} total.",
                summary.retention_deleted,
                summary.size_deleted,
                total
            );
        }
        if (!summary.blocked_by_today) {
            return;
        }
        if (summary.dry_run) {
            auto_core.log_debug(
                "Dry run. Log cleanup deleted {} file(s). Projected retention deletions: {}. Projected size deletions: {}. Remaining size would be {} bytes. Today's protected logs prevented reaching the size limit.",
                total,
                summary.projected_retention,
                summary.projected_size,
                summary.remaining_bytes
            );
            return;
        }
        auto_core.log_debug(
            "Log cleanup deleted {} file(s). Remaining size is {} bytes. Today's protected logs prevented reaching the size limit.",
            total,
            summary.remaining_bytes
        );
    }

    void log_cleanup_result(const ac::logging_cleanup::Result& result) {
        log_cleanup_summary(result.summary);
        switch (result.status) {
        case ac::logging_cleanup::Status::finished:
            return;
        case ac::logging_cleanup::Status::busy:
            auto_core.log_debug("{}", result.detail);
            return;
        case ac::logging_cleanup::Status::failed:
        case ac::logging_cleanup::Status::incomplete:
            auto_core.log_alert("{}", result.detail);
            return;
        }
    }

    void run_passes(const HANDLE stop) {
        try {
            for (;;) {
                try {
                    log_cleanup_result(ac::logging_cleanup::run(
                        {.dry_run = false}
                    ));
                }
                catch (const std::exception& exception) {
                    auto_core.log_alert(
                        "Log cleanup failed: {}",
                        exception.what()
                    );
                }
                catch (...) {
                    auto_core.log_alert("Log cleanup failed.");
                }
                if (stop == nullptr) {
                    break;
                }
                const DWORD wait = WaitForSingleObject(
                    stop,
                    milliseconds_until_next_local_midnight()
                );
                if (wait != WAIT_TIMEOUT) {
                    break;
                }
            }
        }
        catch (const std::exception& exception) {
            auto_core.log_alert(
                "Log cleanup failed: {}",
                exception.what()
            );
        }
        catch (...) {
            auto_core.log_alert("Log cleanup failed.");
        }
    }

} // namespace

LoggingCleanupWorker::LoggingCleanupWorker() noexcept = default;

LoggingCleanupWorker::~LoggingCleanupWorker() {
    request_stop();
    join();
}

LoggingCleanupWorker::LoggingCleanupWorker(
    LoggingCleanupWorker&& other
) noexcept = default;

LoggingCleanupWorker& LoggingCleanupWorker::operator=(
    LoggingCleanupWorker&& other
) noexcept = default;

void LoggingCleanupWorker::request_stop() noexcept {
    if (state_ != nullptr && state_->stop != nullptr) {
        SetEvent(state_->stop);
    }
}

void LoggingCleanupWorker::join() {
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

LoggingCleanupWorker start_logging_cleanup() {
    LoggingCleanupWorker worker;
    worker.state_ = std::make_unique<LoggingCleanupWorker::State>();
    worker.state_->stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    const HANDLE stop = worker.state_->stop;
    worker.state_->thread = std::jthread {[stop] {
        run_passes(stop);
    }};
    return worker;
}
