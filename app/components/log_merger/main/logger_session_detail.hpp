/**
 * \file logger_session_detail.hpp
 * \brief Hosted-versus-manual and schedule decisions for logger_ac.exe.
 *
 * `merge_once` stays outside these decisions.
 */
#pragma once

#include <cstdint>
#include <string_view>

namespace ac::logger::detail {

    /** Win32 `ERROR_FILE_NOT_FOUND`. A missing pipe is the manual-mode signal. */
    inline constexpr unsigned long pipe_absent_error = 2;

    enum class LaunchKind {
        hosted,
        manual,
        failed
    };

    [[nodiscard]]
    inline LaunchKind classify_pipe_open(
        const bool opened,
        const unsigned long error
    ) noexcept {
        if (opened) {
            return LaunchKind::hosted;
        }
        if (error == pipe_absent_error) {
            return LaunchKind::manual;
        }
        return LaunchKind::failed;
    }

    struct HostedSchedule {
        bool merge_before_wait = false;
        bool wait_forever = true;
    };

    [[nodiscard]]
    inline HostedSchedule schedule_for(
        const std::uint64_t merge_interval_seconds
    ) noexcept {
        if (merge_interval_seconds == 0) {
            return {};
        }
        return {.merge_before_wait = true, .wait_forever = false};
    }

    enum class WaitOutcome {
        shutdown,
        interval,
        failed,
        unexpected
    };

    inline constexpr unsigned long wait_object_0 = 0;
    inline constexpr unsigned long wait_timeout = 0x102;
    inline constexpr unsigned long wait_failed = 0xFFFFFFFFu;

    /** Largest wait below Win32 `INFINITE`, so a timeout can still elapse. */
    inline constexpr std::uint64_t max_wait_milliseconds = 0xFFFFFFFEu;

    [[nodiscard]]
    inline std::uint64_t interval_milliseconds(
        std::uint64_t seconds
    ) noexcept {
        constexpr auto max_seconds = max_wait_milliseconds / 1000;
        if (seconds > max_seconds) {
            seconds = max_seconds;
        }
        return seconds * 1000;
    }

    [[nodiscard]]
    inline WaitOutcome classify_wait(const unsigned long result) noexcept {
        if (result == wait_object_0) {
            return WaitOutcome::shutdown;
        }
        if (result == wait_timeout) {
            return WaitOutcome::interval;
        }
        if (result == wait_failed) {
            return WaitOutcome::failed;
        }
        return WaitOutcome::unexpected;
    }

    /** The hosted process never merges again after shutdown is signaled. */
    [[nodiscard]]
    inline bool hosted_shutdown_merges() noexcept {
        return false;
    }

    [[nodiscard]]
    inline bool is_once_argument(const std::wstring_view argument) noexcept {
        return argument == L"--once";
    }

    [[nodiscard]]
    inline bool is_shutdown_argument(const std::wstring_view argument) noexcept {
        return argument == L"--shutdown";
    }

} // namespace ac::logger::detail
