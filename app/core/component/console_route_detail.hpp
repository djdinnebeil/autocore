/**
 * \file console_route_detail.hpp
 * \brief File and console sinks for one component message.
 *
 * Classification happens before routing. `disable_all` suppresses
 * logging-controlled output and skips the disabled-family notice.
 * User-facing calls keep the console. Their file output follows the
 * same file gate as logging-controlled messages.
 */
#pragma once

namespace ac::component_detail {

    struct SinkRequest {
        bool disable_all = false;
        bool component_logging = true;
        bool write_logs_to_files = true;
        bool write_logs_to_console = false;
        bool user_facing = false;
        bool main_subset = false;
        bool notice_already_emitted = false;
    };

    struct SinkDecision {
        bool emit_notice = false;
        bool write_files = false;
        bool write_main = false;
        bool write_console = false;
    };

    [[nodiscard]]
    inline SinkDecision decide_sinks(const SinkRequest& request) noexcept {
        SinkDecision decision;
        const bool logging_active =
            !request.disable_all && request.component_logging;
        decision.emit_notice =
            !request.disable_all &&
            !request.component_logging &&
            !request.notice_already_emitted;
        const bool files = logging_active && request.write_logs_to_files;
        if (request.user_facing) {
            decision.write_files = files;
            decision.write_main = files && request.main_subset;
            decision.write_console = true;
            return decision;
        }
        if (!logging_active) {
            return decision;
        }
        decision.write_files = files;
        decision.write_main = files && request.main_subset;
        decision.write_console = request.write_logs_to_console;
        return decision;
    }

} // namespace ac::component_detail
