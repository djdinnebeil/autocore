/**
 * \file logger_shutdown_detail.hpp
 * \brief Logger is shut down after other children, with a fresh timeout.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

namespace ac::main::components::detail {

    inline constexpr std::string_view logger_component_name = "logger";

    [[nodiscard]]
    inline bool is_logger(const std::string_view name) noexcept {
        return name == logger_component_name;
    }

    /**
     * Phase 2 waits this long from the moment logger is signaled.
     * It is not the time left after phase 1.
     */
    [[nodiscard]]
    inline std::chrono::milliseconds logger_phase_timeout(
        const std::chrono::milliseconds configured
    ) noexcept {
        return configured;
    }

    /** Interval 0 does not start the hosted periodic logger. */
    [[nodiscard]]
    inline bool host_periodic_logger(
        const std::uint64_t merge_interval_seconds
    ) noexcept {
        return merge_interval_seconds >= 1;
    }

    /** `--once` is launched only after the hosted logger has exited. */
    [[nodiscard]]
    inline bool launch_shutdown_once(
        const bool merge_logs_on_shutdown
    ) noexcept {
        return merge_logs_on_shutdown;
    }

} // namespace ac::main::components::detail
