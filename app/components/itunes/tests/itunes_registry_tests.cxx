#include "catch_amalgamated.hpp"

#include <array>
#include <string>
#include <vector>

import itunes_protocol;
import itunes_registry;

TEST_CASE("iTunes registry contains every canonical command", "[itunes][registry][unit]") {
    std::array<int, 6> invocations {};
    auto registry = create_itunes_command_registry({
        .print_songs = [&] { ++invocations[0]; },
        .next_song = [&] { ++invocations[1]; },
        .print_next_up = [&] { ++invocations[2]; },
        .play_pause = [&] { ++invocations[3]; },
        .stop_song = [&] { ++invocations[4]; },
        .remove_song = [&] { ++invocations[5]; }
    });

    const std::vector<std::string> expected {
        "itunes_next_song",
        "itunes_play_pause",
        "itunes_print_next_up",
        "itunes_print_songs",
        "itunes_remove_song",
        "itunes_stop_song"
    };

    CHECK(registry.registered_names() == expected);
    for (const auto command : itunes::commands::all) {
        CHECK(registry.contains(command.name));
    }
    CHECK_FALSE(registry.contains("iTunes_next_song"));
    CHECK_FALSE(registry.contains("unknown_itunes_command"));
}

TEST_CASE("iTunes registry dispatches each command to its action", "[itunes][registry][unit]") {
    std::array<int, 6> invocations {};
    auto registry = create_itunes_command_registry({
        .print_songs = [&] { ++invocations[0]; },
        .next_song = [&] { ++invocations[1]; },
        .print_next_up = [&] { ++invocations[2]; },
        .play_pause = [&] { ++invocations[3]; },
        .stop_song = [&] { ++invocations[4]; },
        .remove_song = [&] { ++invocations[5]; }
    });

    for (const auto command : itunes::commands::all) {
        const auto action = registry.resolve(command.name);
        REQUIRE(action);
        action();
    }

    const std::array<int, 6> expected {1, 1, 1, 1, 1, 1};
    CHECK(invocations == expected);
    CHECK_FALSE(registry.resolve("unknown_itunes_command"));
}
