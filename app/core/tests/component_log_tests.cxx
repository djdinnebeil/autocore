#include "catch_amalgamated.hpp"
#include "../component/component_log_format_detail.hpp"
#include "../component/console_route_detail.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

import auto_core.core.clock;
import auto_core.core.component;
import auto_core.core.logging.config;

namespace {

    std::string read_text(const std::filesystem::path& path) {
        std::ifstream input(path);
        return std::string {
            std::istreambuf_iterator<char> {input},
            std::istreambuf_iterator<char> {}
        };
    }

    std::string line_with(
        const std::string_view text,
        const std::string_view needle
    ) {
        std::size_t start = 0;
        while (start <= text.size()) {
            const std::size_t end = text.find('\n', start);
            const std::size_t stop =
                end == std::string_view::npos ? text.size() : end;
            const std::string_view line = text.substr(start, stop - start);
            if (line.find(needle) != std::string_view::npos) {
                return std::string {line};
            }
            if (end == std::string_view::npos) {
                break;
            }
            start = end + 1;
        }
        return {};
    }

    std::string capture_stdout(const auto& write) {
        std::ostringstream captured;
        auto* previous = std::cout.rdbuf(captured.rdbuf());
        write();
        std::cout.rdbuf(previous);
        return captured.str();
    }

}

TEST_CASE(
    "Persisted component records include their event type",
    "[component][logging][format]"
) {
    using ac::component_detail::MessageClass;
    using ac::component_detail::message_class_name;

    constexpr std::string_view timestamp = "2026-10-16 23:20:25.049";
    constexpr std::string_view message = "message";

    for (const MessageClass message_class : {
        MessageClass::print,
        MessageClass::debug,
        MessageClass::info,
        MessageClass::alert,
        MessageClass::trace
    }) {
        const std::string_view event_type = message_class_name(message_class);
        std::ostringstream output;
        bool line_open = false;

        REQUIRE(ac::component_detail::write_log_record(
            output,
            line_open,
            timestamp,
            event_type,
            message,
            true
        ));
        CHECK(output.str() ==
            '[' + std::string {timestamp} + "] [" +
            std::string {event_type} + "] message\n");
        CHECK_FALSE(line_open);
    }

    std::ostringstream session_output;
    bool session_line_open = false;
    REQUIRE(ac::component_detail::write_log_record(
        session_output,
        session_line_open,
        timestamp,
        {},
        "Session started",
        true
    ));
    CHECK(session_output.str() ==
        "[2026-10-16 23:20:25.049] Session started\n");
}

TEST_CASE(
    "Component routes local logs",
    "[component][logging]"
) {
    const std::string name = "log_route_probe";
    const auto directory =
        ac::logging::config::components_directory() / name;
    std::filesystem::remove_all(directory);

    const std::string console = capture_stdout([&] {
        ac::Component component {name};
        component.log_debug("only-comprehensive");
        component.log_info("both-main");
        component.log_alert("both-print");
        component.print("both-console");
        component.lognl_debug("open-only");
        component.log_debug(" tail");
        component.lognl_info("Choice: ");
        component.log_info("2");
    });

    const auto date = ac::clock::get_local_datetime().date_iso;
    const auto comprehensive = read_text(
        directory / (date + "_" + name + ".log")
    );

    CHECK(console == "both-print\nboth-console\n");
    CHECK(line_with(comprehensive, "only-comprehensive").empty());
    CHECK(line_with(comprehensive, "open-only tail").empty());

    const auto choice = line_with(comprehensive, "Choice: 2");
    CHECK_FALSE(choice.empty());
    CHECK(choice.find("] [info] Choice: 2") != std::string::npos);
    CHECK(line_with(comprehensive, "both-main").find("] [info] both-main") !=
        std::string::npos);
    CHECK(line_with(comprehensive, "both-print").find("] [alert] both-print") !=
        std::string::npos);
    CHECK(line_with(comprehensive, "both-console").find("] [print] both-console") !=
        std::string::npos);

    CHECK(line_with(comprehensive, "Session started").find("] Session started") !=
        std::string::npos);

    const std::string nl_print_console = capture_stdout([&] {
        ac::Component component {name + "_nl_print"};
        component.lognl_alert("nl-print");
    });
    const std::string printnl_console = capture_stdout([&] {
        ac::Component component {name + "_printnl"};
        component.printnl("nl-console");
    });

    const auto nl_print_dir =
        ac::logging::config::components_directory() / (name + "_nl_print");
    const auto printnl_dir =
        ac::logging::config::components_directory() / (name + "_printnl");
    const auto nl_print_log = read_text(
        nl_print_dir / (date + "_" + name + "_nl_print.log")
    );
    const auto printnl_log = read_text(
        printnl_dir / (date + "_" + name + "_printnl.log")
    );

    CHECK(nl_print_console == "nl-print");
    CHECK(printnl_console == "nl-console");
    CHECK(line_with(nl_print_log, "nl-print").find("] [alert] nl-print") !=
        std::string::npos);
    CHECK(line_with(printnl_log, "nl-console").find("] [print] nl-console") !=
        std::string::npos);

    std::filesystem::remove_all(directory);
    std::filesystem::remove_all(nl_print_dir);
    std::filesystem::remove_all(printnl_dir);
}

TEST_CASE(
    "Sink routing follows severity and alert console",
    "[component][logging]"
) {
    using ac::component_detail::LogLevel;
    using ac::component_detail::MessageClass;
    using ac::component_detail::SinkRequest;
    using ac::component_detail::decide_sinks;

    const SinkRequest logging_on {
        .disable_all = false,
        .component_logging = true,
        .level = LogLevel::info,
        .write_logs_to_files = true,
        .write_logs_to_console = false,
        .write_alerts_to_console = true,
        .message = MessageClass::info
    };
    const auto files_only = decide_sinks(logging_on);
    CHECK_FALSE(files_only.emit_notice);
    CHECK(files_only.write_files);
    CHECK_FALSE(files_only.write_console);

    SinkRequest console_too = logging_on;
    console_too.write_logs_to_console = true;
    const auto both = decide_sinks(console_too);
    CHECK(both.write_files);
    CHECK(both.write_console);

    SinkRequest no_files = logging_on;
    no_files.write_logs_to_files = false;
    no_files.write_logs_to_console = true;
    const auto console_without_files = decide_sinks(no_files);
    CHECK_FALSE(console_without_files.write_files);
    CHECK(console_without_files.write_console);

    SinkRequest killed = logging_on;
    killed.disable_all = true;
    killed.write_logs_to_console = true;
    killed.write_alerts_to_console = true;
    killed.message = MessageClass::alert;
    const auto suppressed = decide_sinks(killed);
    CHECK_FALSE(suppressed.emit_notice);
    CHECK_FALSE(suppressed.write_files);
    CHECK_FALSE(suppressed.write_console);

    SinkRequest family_off = logging_on;
    family_off.component_logging = false;
    family_off.write_logs_to_files = false;
    family_off.write_logs_to_console = false;
    const auto notice = decide_sinks(family_off);
    CHECK(notice.emit_notice);
    CHECK_FALSE(notice.write_files);
    CHECK_FALSE(notice.write_console);

    SinkRequest noticed = family_off;
    noticed.notice_already_emitted = true;
    CHECK_FALSE(decide_sinks(noticed).emit_notice);

    SinkRequest printed = family_off;
    printed.message = MessageClass::print;
    printed.notice_already_emitted = true;
    const auto printed_off = decide_sinks(printed);
    CHECK(printed_off.write_console);
    CHECK_FALSE(printed_off.write_files);

    SinkRequest printed_files = logging_on;
    printed_files.message = MessageClass::print;
    const auto printed_on = decide_sinks(printed_files);
    CHECK(printed_on.write_console);
    CHECK(printed_on.write_files);

    SinkRequest print_copy_off = printed_files;
    print_copy_off.log_console_output = false;
    const auto print_not_copied = decide_sinks(print_copy_off);
    CHECK(print_not_copied.write_console);
    CHECK_FALSE(print_not_copied.write_files);

    SinkRequest print_unaffected = printed_files;
    print_unaffected.level = LogLevel::alert;
    print_unaffected.write_logs_to_console = true;
    print_unaffected.write_alerts_to_console = false;
    const auto print_still_copied = decide_sinks(print_unaffected);
    CHECK(print_still_copied.write_console);
    CHECK(print_still_copied.write_files);

    SinkRequest print_without_files = logging_on;
    print_without_files.message = MessageClass::print;
    print_without_files.write_logs_to_files = false;
    print_without_files.write_logs_to_console = false;
    print_without_files.write_alerts_to_console = false;
    const auto print_console = decide_sinks(print_without_files);
    CHECK(print_console.write_console);
    CHECK_FALSE(print_console.write_files);

    SinkRequest debug_at_info = logging_on;
    debug_at_info.message = MessageClass::debug;
    debug_at_info.level = LogLevel::info;
    const auto hidden_debug = decide_sinks(debug_at_info);
    CHECK_FALSE(hidden_debug.write_files);
    CHECK_FALSE(hidden_debug.write_console);

    SinkRequest trace_at_info = logging_on;
    trace_at_info.message = MessageClass::trace;
    trace_at_info.level = LogLevel::info;
    const auto trace_hidden_at_info = decide_sinks(trace_at_info);
    CHECK_FALSE(trace_hidden_at_info.write_files);
    CHECK_FALSE(trace_hidden_at_info.write_console);

    SinkRequest trace_at_debug = logging_on;
    trace_at_debug.message = MessageClass::trace;
    trace_at_debug.level = LogLevel::debug;
    const auto trace_hidden_at_debug = decide_sinks(trace_at_debug);
    CHECK_FALSE(trace_hidden_at_debug.write_files);

    SinkRequest trace_at_trace = logging_on;
    trace_at_trace.message = MessageClass::trace;
    trace_at_trace.level = LogLevel::trace;
    const auto trace_enabled = decide_sinks(trace_at_trace);
    CHECK(trace_enabled.write_files);
    CHECK_FALSE(trace_enabled.write_console);

    SinkRequest info_at_alert = logging_on;
    info_at_alert.message = MessageClass::info;
    info_at_alert.level = LogLevel::alert;
    const auto hidden_info = decide_sinks(info_at_alert);
    CHECK_FALSE(hidden_info.write_files);

    SinkRequest alert_only = logging_on;
    alert_only.message = MessageClass::alert;
    alert_only.level = LogLevel::alert;
    alert_only.write_logs_to_console = false;
    alert_only.write_alerts_to_console = true;
    const auto alert_console = decide_sinks(alert_only);
    CHECK(alert_console.write_files);
    CHECK(alert_console.write_console);

    SinkRequest alert_both = alert_only;
    alert_both.write_logs_to_console = true;
    alert_both.write_alerts_to_console = true;
    const auto alert_once = decide_sinks(alert_both);
    CHECK(alert_once.write_console);
    CHECK(alert_once.write_files);

    SinkRequest alert_quiet = alert_only;
    alert_quiet.write_alerts_to_console = false;
    const auto alert_file = decide_sinks(alert_quiet);
    CHECK(alert_file.write_files);
    CHECK_FALSE(alert_file.write_console);

    SinkRequest killed_print = logging_on;
    killed_print.disable_all = true;
    killed_print.message = MessageClass::print;
    const auto print_when_disabled = decide_sinks(killed_print);
    CHECK(print_when_disabled.write_console);
    CHECK_FALSE(print_when_disabled.write_files);
}

TEST_CASE(
    "Component construction does not open log files",
    "[component][logging]"
) {
    const std::string name = "log_defer_probe";
    const auto directory =
        ac::logging::config::components_directory() / name;
    std::filesystem::remove_all(directory);

    {
        ac::Component component {name};
        CHECK_FALSE(std::filesystem::exists(directory));
        component.log_info("after-construct");
    }

    const bool file_logging =
        !ac::logging::config::disable_all() &&
        ac::logging::config::component_logging_default() &&
        ac::logging::config::write_logs_to_files();
    if (file_logging) {
        CHECK(std::filesystem::exists(directory));
    }
    else {
        CHECK_FALSE(std::filesystem::exists(directory));
    }
    std::filesystem::remove_all(directory);
}
