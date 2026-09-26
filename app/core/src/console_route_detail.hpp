/**
 * \file console_route_detail.hpp
 * \brief Whether one component message also goes to the console.
 *
 * Explicit console routes and `write_logs_to_console` share one write.
 */
#pragma once

namespace ac::component_detail {

    [[nodiscard]]
    inline int console_write_count(
        const bool explicit_console,
        const bool write_logs_to_console
    ) noexcept {
        return (explicit_console || write_logs_to_console) ? 1 : 0;
    }

} // namespace ac::component_detail
