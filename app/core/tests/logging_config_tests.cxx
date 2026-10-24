#include "catch_amalgamated.hpp"
#include "../config/logging_config_detail.hpp"

namespace detail = ac::logging::config::detail;

TEST_CASE("Logging configuration applies explicit values", "[logging-config][unit]") {
    const auto settings = detail::resolve_logging(
        {
            .disable_all = "on",
            .level = "info",
            .log_console_output = "off",
            .write_logs_to_files = "off",
            .write_logs_to_console = "on",
            .write_alerts_to_console = "off",
            .component_logging_default = "off",
            .max_directory_mb = "0",
            .retention_days = "7",
            .directory = R"(D:\logs)",
            .experimental = "off"
        },
        R"(C:\default-logs)",
        R"(C:\app)"
    );

    CHECK(settings.disable_all);
    CHECK_FALSE(settings.experimental);
    CHECK(settings.level == detail::LogLevel::info);
    CHECK_FALSE(settings.log_console_output);
    CHECK_FALSE(settings.write_logs_to_files);
    CHECK(settings.write_logs_to_console);
    CHECK_FALSE(settings.write_alerts_to_console);
    CHECK_FALSE(settings.component_logging_default);
    CHECK(settings.max_directory_mb == 0);
    CHECK(settings.retention_days == 7);
    CHECK(settings.directory == R"(D:\logs)");
    CHECK(settings.components_directory ==
        std::filesystem::path {R"(D:\logs\components)"});
    CHECK(settings.report.empty());
}

TEST_CASE("Logging defaults keep debug level and alert console", "[logging-config][unit]") {
    const auto settings = detail::resolve_logging(
        {},
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK_FALSE(settings.disable_all);
    CHECK_FALSE(settings.experimental);
    CHECK(settings.level == detail::LogLevel::debug);
    CHECK(settings.log_console_output);
    CHECK(settings.write_logs_to_files);
    CHECK_FALSE(settings.write_logs_to_console);
    CHECK(settings.write_alerts_to_console);
    CHECK(settings.component_logging_default);
    CHECK(settings.max_directory_mb == 100);
    CHECK(settings.retention_days == 30);
    const auto disable_all = settings.report.find("disable_all missing or invalid");
    const auto experimental =
        settings.report.find("experimental missing or invalid");
    const auto max_directory_mb =
        settings.report.find("max_directory_mb missing or invalid");
    const auto retention_days =
        settings.report.find("retention_days missing or invalid");
    const auto level = settings.report.find("level missing or invalid");
    const auto console_output = settings.report.find("log_console_output missing or invalid");
    const auto files = settings.report.find("write_logs_to_files missing or invalid");
    const auto console = settings.report.find("write_logs_to_console missing or invalid");
    const auto alerts = settings.report.find("write_alerts_to_console missing or invalid");
    const auto family = settings.report.find("component_logging_default missing or invalid");
    CHECK(disable_all != std::string::npos);
    CHECK(experimental != std::string::npos);
    CHECK(max_directory_mb != std::string::npos);
    CHECK(retention_days != std::string::npos);
    CHECK(level != std::string::npos);
    CHECK(console_output != std::string::npos);
    CHECK(files != std::string::npos);
    CHECK(console != std::string::npos);
    CHECK(alerts != std::string::npos);
    CHECK(family != std::string::npos);
    CHECK(disable_all < max_directory_mb);
    CHECK(max_directory_mb < retention_days);
    CHECK(retention_days < level);
    CHECK(console_output < files);
    CHECK(files < console);
    CHECK(console < alerts);
    CHECK(alerts < family);
    CHECK(family < experimental);
}

TEST_CASE("Logging booleans accept only on and off", "[logging-config][unit]") {
    const auto on = detail::resolve_logging(
        {
            .disable_all = "on",
            .write_logs_to_files = "on",
            .experimental = "on"
        },
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto off = detail::resolve_logging(
        {
            .disable_all = "off",
            .write_logs_to_files = "off",
            .experimental = "off"
        },
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto truth = detail::resolve_logging(
        {.disable_all = "true", .component_logging_default = "true"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto falsity = detail::resolve_logging(
        {.disable_all = "false", .component_logging_default = "false"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto invalid = detail::resolve_logging(
        {
            .disable_all = "yes",
            .write_logs_to_files = "yes",
            .experimental = "yes"
        },
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK(on.disable_all);
    CHECK(on.experimental);
    CHECK(on.write_logs_to_files);
    CHECK_FALSE(off.disable_all);
    CHECK_FALSE(off.experimental);
    CHECK_FALSE(off.write_logs_to_files);
    CHECK_FALSE(truth.disable_all);
    CHECK(truth.component_logging_default);
    CHECK_FALSE(falsity.disable_all);
    CHECK(falsity.component_logging_default);
    CHECK(truth.report.find("disable_all missing or invalid") !=
        std::string::npos);
    CHECK_FALSE(invalid.disable_all);
    CHECK_FALSE(invalid.experimental);
    CHECK(invalid.write_logs_to_files);
    CHECK(invalid.report.find("disable_all missing or invalid") !=
        std::string::npos);
    CHECK(invalid.report.find("experimental missing or invalid") !=
        std::string::npos);
    CHECK(invalid.report.find("write_logs_to_files missing or invalid") !=
        std::string::npos);
}

TEST_CASE("level accepts only trace, debug, info, and alert", "[logging-config][unit]") {
    const auto trace_level = detail::resolve_logging(
        {.level = "trace"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto debug_level = detail::resolve_logging(
        {.level = "debug"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto info_level = detail::resolve_logging(
        {.level = "info"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto alert_level = detail::resolve_logging(
        {.level = "alert"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto invalid = detail::resolve_logging(
        {.level = "off"},
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK(trace_level.level == detail::LogLevel::trace);
    CHECK(debug_level.level == detail::LogLevel::debug);
    CHECK(info_level.level == detail::LogLevel::info);
    CHECK(alert_level.level == detail::LogLevel::alert);
    CHECK(invalid.level == detail::LogLevel::debug);
    CHECK(invalid.report.find("level missing or invalid") !=
        std::string::npos);
}

TEST_CASE("Storage integers accept zero and reject negatives", "[logging-config][unit]") {
    const auto zero = detail::resolve_logging(
        {.max_directory_mb = "0", .retention_days = "0"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto invalid = detail::resolve_logging(
        {.max_directory_mb = "-1", .retention_days = "30a"},
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK(zero.max_directory_mb == 0);
    CHECK(zero.retention_days == 0);
    CHECK(invalid.max_directory_mb == 100);
    CHECK(invalid.retention_days == 30);
    CHECK(invalid.report.find("max_directory_mb missing or invalid") !=
        std::string::npos);
    CHECK(invalid.report.find("retention_days missing or invalid") !=
        std::string::npos);
}

TEST_CASE("Family logging uses the supplied fallback", "[logging-config][unit]") {
    using detail::LoggingFallback;
    const std::optional<std::string_view> missing;
    const std::optional<std::string_view> invalid {"yes"};
    const std::optional<std::string_view> on {"on"};
    const std::optional<std::string_view> off {"off"};
    const std::optional<std::string_view> unrecognized {"true"};

    CHECK(detail::resolve_component_logging(
        LoggingFallback::global_default, on, false));
    CHECK_FALSE(detail::resolve_component_logging(
        LoggingFallback::global_default, off, true));
    CHECK_FALSE(detail::resolve_component_logging(
        LoggingFallback::off, unrecognized, true));
    CHECK(detail::resolve_component_logging(
        LoggingFallback::global_default, unrecognized, true));
    CHECK(detail::resolve_component_logging(
        LoggingFallback::global_default, missing, true));
    CHECK_FALSE(detail::resolve_component_logging(
        LoggingFallback::global_default, invalid, false));
    CHECK_FALSE(detail::resolve_component_logging(
        LoggingFallback::off, missing, true));
    CHECK_FALSE(detail::resolve_component_logging(
        LoggingFallback::off, invalid, true));
}

TEST_CASE("Logging console flag accepts documented booleans", "[logging-config][unit]") {
    const auto on = detail::resolve_logging(
        {.write_logs_to_console = "on"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto off = detail::resolve_logging(
        {.write_logs_to_console = "off"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto truth = detail::resolve_logging(
        {.write_logs_to_console = "true"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto falsity = detail::resolve_logging(
        {.write_logs_to_console = "false"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto invalid = detail::resolve_logging(
        {.write_logs_to_console = "yes"},
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK(on.write_logs_to_console);
    CHECK_FALSE(off.write_logs_to_console);
    CHECK_FALSE(truth.write_logs_to_console);
    CHECK_FALSE(falsity.write_logs_to_console);
    CHECK(truth.report.find("write_logs_to_console missing or invalid") !=
        std::string::npos);
    CHECK_FALSE(invalid.write_logs_to_console);
    CHECK(invalid.report.find("write_logs_to_console missing or invalid") !=
        std::string::npos);
}

TEST_CASE("Logging directory logs is installation-root logs", "[logging-config][unit]") {
    const auto settings = detail::resolve_logging(
        {.directory = "logs"},
        R"(C:\default)",
        R"(C:\app)"
    );

    CHECK(settings.directory == std::filesystem::path {R"(C:\app\logs)"});
    CHECK(settings.components_directory ==
        std::filesystem::path {R"(C:\app\logs\components)"});
}

TEST_CASE("Relative logging directories use the installation root", "[logging-config][unit]") {
    const auto settings = detail::resolve_logging(
        {.directory = R"(logs\components\..\current)"},
        R"(C:\default)",
        R"(C:\app)"
    );

    CHECK(settings.directory ==
        std::filesystem::path {R"(C:\app\logs\current)"});
}

TEST_CASE("Empty logging directory uses paths default", "[logging-config][unit]") {
    const auto settings = detail::resolve_logging(
        {.directory = ""},
        R"(C:\default)",
        R"(C:\app)"
    );

    CHECK(settings.directory == R"(C:\default)");
}
