/**
 * \file console_route_detail.hpp
 * \brief File and console sinks for one component message.
 *
 * `MessageClass` matches `ac::MessageClass`. `LogLevel` matches
 * `ac::logging::config::LogLevel`. Classification is the caller's class.
 * `disable_all` suppresses diagnostic output, skips the
 * disabled-family notice, and suppresses the `print` file copy.
 * `MessageClass::print` keeps the console. Its file copy requires
 * active logging, `write_logs_to_files`, and `log_console_output`.
 * `write_logs_to_files` is the master file-output gate.
 * `log_console_output` controls whether `print` participates in that
 * sink. `write_logs_to_console` mirrors enabled diagnostic logs.
 * `write_alerts_to_console` shows enabled alerts, once when both
 * console flags are on.
 */
#pragma once

#include <string_view>

namespace ac::component_detail {

    enum class MessageClass : unsigned char {
        trace = 0,
        debug = 1,
        info = 2,
        alert = 3,
        print = 4
    };

    enum class LogLevel : unsigned char {
        trace = 0,
        debug = 1,
        info = 2,
        alert = 3
    };

    [[nodiscard]]
    inline constexpr std::string_view message_class_name(
        const MessageClass message
    ) noexcept {
        switch (message) {
        case MessageClass::print:
            return "print";
        case MessageClass::debug:
            return "debug";
        case MessageClass::info:
            return "info";
        case MessageClass::alert:
            return "alert";
        case MessageClass::trace:
            return "trace";
        }
        return {};
    }

    struct SinkRequest {
        bool disable_all = false;
        bool component_logging = true;
        LogLevel level = LogLevel::info;
        bool log_console_output = true;
        bool write_logs_to_files = true;
        bool write_logs_to_console = false;
        bool write_alerts_to_console = true;
        MessageClass message = MessageClass::info;
        bool notice_already_emitted = false;
    };

    struct SinkDecision {
        bool emit_notice = false;
        bool write_files = false;
        bool write_console = false;
    };

    [[nodiscard]]
    inline LogLevel message_level(const MessageClass message) noexcept {
        switch (message) {
        case MessageClass::trace:
            return LogLevel::trace;
        case MessageClass::debug:
            return LogLevel::debug;
        case MessageClass::info:
            return LogLevel::info;
        case MessageClass::alert:
        case MessageClass::print:
            return LogLevel::alert;
        }
        return LogLevel::alert;
    }

    [[nodiscard]]
    inline bool severity_enabled(
        const LogLevel configured,
        const MessageClass message
    ) noexcept {
        if (message == MessageClass::print) {
            return true;
        }
        if (static_cast<unsigned char>(message_level(message)) <
            static_cast<unsigned char>(configured)) {
            return false;
        }
        return true;
    }

    [[nodiscard]]
    inline SinkDecision decide_sinks(const SinkRequest& request) noexcept {
        SinkDecision decision;
        const bool print_message = request.message == MessageClass::print;
        const bool logging_active =
            !request.disable_all && request.component_logging;
        decision.emit_notice =
            !request.disable_all &&
            !request.component_logging &&
            !request.notice_already_emitted;
        if (print_message) {
            decision.write_files =
                logging_active &&
                request.write_logs_to_files &&
                request.log_console_output;
            decision.write_console = true;
            return decision;
        }
        if (!logging_active ||
            !severity_enabled(
                request.level,
                request.message
            )) {
            return decision;
        }
        decision.write_files = request.write_logs_to_files;
        if (request.message == MessageClass::alert) {
            decision.write_console =
                request.write_logs_to_console ||
                request.write_alerts_to_console;
        }
        else {
            decision.write_console = request.write_logs_to_console;
        }
        return decision;
    }

} // namespace ac::component_detail
