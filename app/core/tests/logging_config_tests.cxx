#include "catch_amalgamated.hpp"
#include "../src/logging_config_detail.hpp"

namespace detail = ac::logging::config::detail;

TEST_CASE("Logger configuration applies explicit values", "[logging-config][unit]") {
    const auto settings = detail::resolve(
        {
            .write_to_console = "true",
            .directory = R"(D:\logs)"
        },
        R"(C:\default-logs)",
        R"(C:\app)"
    );

    CHECK(settings.write_to_console);
    CHECK(settings.directory == R"(D:\logs)");
    CHECK(settings.components_directory ==
        std::filesystem::path {R"(D:\logs\components)"});
}

TEST_CASE("Logger configuration accepts on and off", "[logging-config][unit]") {
    const auto settings = detail::resolve(
        {
            .write_to_console = "on"
        },
        R"(C:\default-logs)",
        R"(C:\app)"
    );

    CHECK(settings.write_to_console);
}

TEST_CASE("Logger configuration accepts false", "[logging-config][unit]") {
    const auto settings = detail::resolve(
        {
            .write_to_console = "false"
        },
        R"(C:\default-logs)",
        R"(C:\app)"
    );

    CHECK_FALSE(settings.write_to_console);
}

TEST_CASE("Logger configuration has efficient defaults", "[logging-config][unit]") {
    const auto settings = detail::resolve(
        {},
        R"(C:\default-logs)",
        R"(C:\app)"
    );

    CHECK_FALSE(settings.write_to_console);
    CHECK(settings.directory == R"(C:\default-logs)");
    CHECK(settings.components_directory ==
        std::filesystem::path {R"(C:\default-logs\components)"});
}

TEST_CASE("Invalid logger booleans use defaults", "[logging-config][unit]") {
    const auto settings = detail::resolve(
        {
            .write_to_console = "yes"
        },
        R"(C:\logs)",
        R"(C:\app)"
    );

    CHECK_FALSE(settings.write_to_console);
    CHECK(settings.report.find("write_to_console missing or invalid") !=
        std::string::npos);
    CHECK(settings.components_directory ==
        std::filesystem::path {R"(C:\logs\components)"});
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
    CHECK(settings.components_directory ==
        std::filesystem::path {R"(C:\app\logs\current\components)"});
}

TEST_CASE("Empty logger directory uses paths default", "[logging-config][unit]") {
    const auto settings = detail::resolve(
        {.directory = ""},
        R"(C:\default)",
        R"(C:\app)"
    );

    CHECK(settings.directory == R"(C:\default)");
    CHECK(settings.components_directory ==
        std::filesystem::path {R"(C:\default\components)"});
}
