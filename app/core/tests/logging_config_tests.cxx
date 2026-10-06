#include "catch_amalgamated.hpp"
#include "../config/logging_config_detail.hpp"

namespace detail = ac::logging::config::detail;

TEST_CASE("Logging configuration applies explicit values", "[logging-config][unit]") {
    const auto settings = detail::resolve_logging(
        {
            .disable_all = "on",
            .write_logs_to_files = "off",
            .write_logs_to_console = "on",
            .log_print_mode = "log",
            .component_logging_default = "off",
            .directory = R"(D:\logs)"
        },
        R"(C:\default-logs)",
        R"(C:\app)"
    );

    CHECK(settings.disable_all);
    CHECK_FALSE(settings.write_logs_to_files);
    CHECK(settings.write_logs_to_console);
    CHECK(settings.log_print_mode == detail::LogPrintMode::log);
    CHECK_FALSE(settings.component_logging_default);
    CHECK(settings.directory == R"(D:\logs)");
    CHECK(settings.components_directory ==
        std::filesystem::path {R"(D:\logs\components)"});
    CHECK(settings.report.find("disable_all = on") != std::string::npos);
    CHECK(settings.report.find("write_logs_to_files = off") != std::string::npos);
    CHECK(settings.report.find("write_logs_to_console = on") !=
        std::string::npos);
    CHECK(settings.report.find("log_print_mode = log") != std::string::npos);
    CHECK(settings.report.find("component_logging_default = off") !=
        std::string::npos);
}

TEST_CASE("Logging defaults keep files on, console off, and print mode", "[logging-config][unit]") {
    const auto settings = detail::resolve_logging(
        {},
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK_FALSE(settings.disable_all);
    CHECK(settings.write_logs_to_files);
    CHECK_FALSE(settings.write_logs_to_console);
    CHECK(settings.log_print_mode == detail::LogPrintMode::print);
    CHECK(settings.component_logging_default);
    const auto disable_all = settings.report.find("disable_all missing or invalid");
    const auto directory = settings.report.find("directory = ");
    const auto files = settings.report.find("write_logs_to_files missing or invalid");
    const auto console = settings.report.find("write_logs_to_console missing or invalid");
    const auto mode = settings.report.find("log_print_mode missing or invalid");
    const auto family = settings.report.find("component_logging_default missing or invalid");
    CHECK(disable_all != std::string::npos);
    CHECK(directory != std::string::npos);
    CHECK(files != std::string::npos);
    CHECK(console != std::string::npos);
    CHECK(mode != std::string::npos);
    CHECK(family != std::string::npos);
    CHECK(disable_all < directory);
    CHECK(directory < files);
    CHECK(files < console);
    CHECK(console < mode);
    CHECK(mode < family);
    CHECK(settings.report.find("write_logs_to_files missing or invalid") !=
        std::string::npos);
    CHECK(settings.report.find("write_logs_to_console missing or invalid") !=
        std::string::npos);
    CHECK(settings.report.find("log_print_mode missing or invalid") !=
        std::string::npos);
    CHECK(settings.report.find("component_logging_default missing or invalid") !=
        std::string::npos);
}

TEST_CASE("Logging booleans accept only on and off", "[logging-config][unit]") {
    const auto on = detail::resolve_logging(
        {.disable_all = "on", .write_logs_to_files = "on"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto off = detail::resolve_logging(
        {.disable_all = "off", .write_logs_to_files = "off"},
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
        {.disable_all = "yes", .write_logs_to_files = "yes"},
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK(on.disable_all);
    CHECK(on.write_logs_to_files);
    CHECK_FALSE(off.disable_all);
    CHECK_FALSE(off.write_logs_to_files);
    CHECK_FALSE(truth.disable_all);
    CHECK(truth.component_logging_default);
    CHECK_FALSE(falsity.disable_all);
    CHECK(falsity.component_logging_default);
    CHECK(truth.report.find("disable_all missing or invalid") !=
        std::string::npos);
    CHECK_FALSE(invalid.disable_all);
    CHECK(invalid.write_logs_to_files);
    CHECK(invalid.report.find("disable_all missing or invalid") !=
        std::string::npos);
    CHECK(invalid.report.find("write_logs_to_files missing or invalid") !=
        std::string::npos);
}

TEST_CASE("log_print_mode accepts only log and print", "[logging-config][unit]") {
    const auto log_mode = detail::resolve_logging(
        {.log_print_mode = "log"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto print_mode = detail::resolve_logging(
        {.log_print_mode = "print"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto invalid = detail::resolve_logging(
        {.log_print_mode = "true"},
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK(log_mode.log_print_mode == detail::LogPrintMode::log);
    CHECK(print_mode.log_print_mode == detail::LogPrintMode::print);
    CHECK(invalid.log_print_mode == detail::LogPrintMode::print);
    CHECK(invalid.report.find("log_print_mode missing or invalid") !=
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
