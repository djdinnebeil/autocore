#include "catch_amalgamated.hpp"
#include "../src/logging_config_detail.hpp"

namespace detail = ac::logging::config::detail;

TEST_CASE("Logger configuration applies explicit values", "[logging-config][unit]") {
    const auto settings = detail::resolve(
        {
            .merge_interval_seconds = "1",
            .merge_logs_on_shutdown = "off",
            .write_logs_to_console = "true",
            .directory = R"(D:\logs)"
        },
        R"(C:\default-logs)",
        R"(C:\app)"
    );

    CHECK(settings.merge_interval_seconds == 1);
    CHECK_FALSE(settings.merge_logs_on_shutdown);
    CHECK(settings.write_logs_to_console);
    CHECK(settings.directory == R"(D:\logs)");
    CHECK(settings.components_directory ==
        std::filesystem::path {R"(D:\logs\components)"});
    CHECK(settings.report.find("merge_interval_seconds = 1") !=
        std::string::npos);
    CHECK(settings.report.find("merge_logs_on_shutdown = off") !=
        std::string::npos);
    CHECK(settings.report.find("write_logs_to_console = on") !=
        std::string::npos);
}

TEST_CASE("Logger interval accepts zero and sixty", "[logging-config][unit]") {
    const auto disabled = detail::resolve(
        {.merge_interval_seconds = "0"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto standard = detail::resolve(
        {.merge_interval_seconds = "60"},
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK(disabled.merge_interval_seconds == 0);
    CHECK(standard.merge_interval_seconds == 60);
}

TEST_CASE("Invalid logger intervals use sixty", "[logging-config][unit]") {
    const auto missing = detail::resolve({}, R"(C:\logs)", R"(C:\app)");
    const auto negative = detail::resolve(
        {.merge_interval_seconds = "-1"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto malformed = detail::resolve(
        {.merge_interval_seconds = "soon"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto trailing = detail::resolve(
        {.merge_interval_seconds = "60s"},
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK(missing.merge_interval_seconds == 60);
    CHECK(negative.merge_interval_seconds == 60);
    CHECK(malformed.merge_interval_seconds == 60);
    CHECK(trailing.merge_interval_seconds == 60);
    CHECK(missing.report.find("merge_interval_seconds missing or invalid") !=
        std::string::npos);
}

TEST_CASE("Logger shutdown merge accepts documented booleans", "[logging-config][unit]") {
    const auto missing = detail::resolve({}, R"(C:\logs)", R"(C:\app)");
    const auto on = detail::resolve(
        {.merge_logs_on_shutdown = "on"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto off = detail::resolve(
        {.merge_logs_on_shutdown = "off"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto truth = detail::resolve(
        {.merge_logs_on_shutdown = "true"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto falsity = detail::resolve(
        {.merge_logs_on_shutdown = "false"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto invalid = detail::resolve(
        {.merge_logs_on_shutdown = "yes"},
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK(missing.merge_logs_on_shutdown);
    CHECK(on.merge_logs_on_shutdown);
    CHECK_FALSE(off.merge_logs_on_shutdown);
    CHECK(truth.merge_logs_on_shutdown);
    CHECK_FALSE(falsity.merge_logs_on_shutdown);
    CHECK(invalid.merge_logs_on_shutdown);
    CHECK(invalid.report.find("merge_logs_on_shutdown missing or invalid") !=
        std::string::npos);
}

TEST_CASE("Logger console flag accepts documented booleans", "[logging-config][unit]") {
    const auto missing = detail::resolve({}, R"(C:\logs)", R"(C:\app)");
    const auto on = detail::resolve(
        {.write_logs_to_console = "on"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto off = detail::resolve(
        {.write_logs_to_console = "off"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto truth = detail::resolve(
        {.write_logs_to_console = "true"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto falsity = detail::resolve(
        {.write_logs_to_console = "false"},
        R"(C:\logs)",
        R"(C:\app)"
    );
    const auto invalid = detail::resolve(
        {.write_logs_to_console = "yes"},
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK_FALSE(missing.write_logs_to_console);
    CHECK(on.write_logs_to_console);
    CHECK_FALSE(off.write_logs_to_console);
    CHECK(truth.write_logs_to_console);
    CHECK_FALSE(falsity.write_logs_to_console);
    CHECK_FALSE(invalid.write_logs_to_console);
    CHECK(missing.report.find("write_logs_to_console = off") !=
        std::string::npos);
}

TEST_CASE("Logger directory logs is installation-root logs", "[logging-config][unit]") {
    const auto settings = detail::resolve(
        {.directory = "logs"},
        R"(C:\default)",
        R"(C:\app)"
    );

    CHECK(settings.directory == std::filesystem::path {R"(C:\app\logs)"});
    CHECK(settings.components_directory ==
        std::filesystem::path {R"(C:\app\logs\components)"});
}

TEST_CASE("Relative logger directories use the installation root", "[logging-config][unit]") {
    const auto settings = detail::resolve(
        {.directory = R"(logs\components\..\current)"},
        R"(C:\default)",
        R"(C:\app)"
    );

    CHECK(settings.directory ==
        std::filesystem::path {R"(C:\app\logs\current)"});
}

TEST_CASE("Empty logger directory uses paths default", "[logging-config][unit]") {
    const auto settings = detail::resolve(
        {.directory = ""},
        R"(C:\default)",
        R"(C:\app)"
    );

    CHECK(settings.directory == R"(C:\default)");
}
