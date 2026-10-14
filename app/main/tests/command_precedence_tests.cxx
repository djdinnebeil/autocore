#include "catch_amalgamated.hpp"

#include <string>
#include <vector>

import command_registry;

namespace {

template<typename Function>
void bind_child_fallback(
    command_registry::Registry& registry,
    std::string name,
    Function action
) {
    if (!registry.contains(name)) {
        registry.add(std::move(name), std::move(action));
    }
}

} // namespace

TEST_CASE("Taskbar command takes precedence over a child fallback", "[commands][precedence][unit]") {
    command_registry::Registry registry;
    int taskbar_invocations = 0;
    int fallback_invocations = 0;

    registry.add("activate_spotify", [&] { ++taskbar_invocations; });
    bind_child_fallback(
        registry,
        "activate_spotify",
        [&] { ++fallback_invocations; }
    );

    const auto action = registry.resolve("activate_spotify");
    REQUIRE(action);
    action();
    CHECK(taskbar_invocations == 1);
    CHECK(fallback_invocations == 0);
    CHECK(registry.registered_names() ==
        std::vector<std::string> {"activate_spotify"});
}

TEST_CASE("Child activation fallback remains without Taskbar command", "[commands][precedence][unit]") {
    command_registry::Registry registry;
    int fallback_invocations = 0;

    bind_child_fallback(
        registry,
        "activate_spotify",
        [&] { ++fallback_invocations; }
    );

    const auto action = registry.resolve("activate_spotify");
    REQUIRE(action);
    action();
    CHECK(fallback_invocations == 1);
}
