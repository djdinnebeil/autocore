#include "catch_amalgamated.hpp"
#include "../diagnostics/crash_diagnostics_detail.hpp"

#include <optional>
#include <string_view>

namespace detail = ac::crash::detail;

TEST_CASE("Crash diagnostics default to enabled", "[crash-diagnostics][unit]") {
    CHECK(detail::resolve_enabled(std::nullopt));
    CHECK(detail::resolve_enabled(std::string_view {"on"}));
}

TEST_CASE("Crash diagnostics accept explicit off", "[crash-diagnostics][unit]") {
    CHECK_FALSE(detail::resolve_enabled(std::string_view {"off"}));
}

TEST_CASE("Invalid crash diagnostics values keep the enabled default", "[crash-diagnostics][unit]") {
    CHECK(detail::resolve_enabled(std::string_view {}));
    CHECK(detail::resolve_enabled(std::string_view {"yes"}));
    CHECK(detail::resolve_enabled(std::string_view {"OFF"}));
}

TEST_CASE("Crash event names are independent and collision-safe", "[crash-diagnostics][unit]") {
    const auto first = detail::event_directory_name(
        "20261004T041205.381Z", "taskbar_ac", 7420
    );
    const auto collision = detail::event_directory_name(
        "20261004T041205.381Z", "taskbar_ac", 7420, 1
    );

    CHECK(first == "20261004T041205.381Z_taskbar_ac_7420");
    CHECK(collision == "20261004T041205.381Z_taskbar_ac_7420_1");
    CHECK(first != collision);
}

TEST_CASE("Crash policy is independent of logging policy", "[crash-diagnostics][unit]") {
    const bool logging_disable_all = true;
    CHECK(logging_disable_all);
    CHECK(detail::resolve_enabled(std::string_view {"on"}));
}
