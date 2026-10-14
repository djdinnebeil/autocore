#include "catch_amalgamated.hpp"

#include <array>
#include <string>
#include <vector>

import itunes_protocol;
import itunes_registry;

TEST_CASE("iTunes registry contains every canonical command", "[itunes][registry][unit]") {
    CHECK(itunes_activation_fallback_message ==
        "Pin or start iTunes, then run taskbar_builder.exe to generate "
        "itunes.map.");
    std::array<int, 7> invocations {};
    auto registry = create_itunes_command_registry({
        .activate_itunes = [&] { ++invocations[0]; },
        .print_songs = [&] { ++invocations[1]; },
        .next_song = [&] { ++invocations[2]; },
        .print_next_up = [&] { ++invocations[3]; },
        .play_pause = [&] { ++invocations[4]; },
        .stop_song = [&] { ++invocations[5]; },
        .remove_song = [&] { ++invocations[6]; }
    });

    const std::vector<std::string> expected {
        "activate_itunes",
        "itunes_next_song",
        "itunes_play_pause",
        "itunes_print_next_up",
        "itunes_print_songs",
        "itunes_remove_song",
        "itunes_stop_song",
        "print_next_up_song_list"
    };

    CHECK(registry.registered_names() == expected);
    for (const auto command : itunes::commands::all) {
        CHECK(registry.contains(command.name));
    }
    CHECK_FALSE(registry.contains("iTunes_next_song"));
    CHECK_FALSE(registry.contains("unknown_itunes_command"));
}

TEST_CASE("iTunes registry dispatches each command to its action", "[itunes][registry][unit]") {
    std::array<int, 7> invocations {};
    auto registry = create_itunes_command_registry({
        .activate_itunes = [&] { ++invocations[0]; },
        .print_songs = [&] { ++invocations[1]; },
        .next_song = [&] { ++invocations[2]; },
        .print_next_up = [&] { ++invocations[3]; },
        .play_pause = [&] { ++invocations[4]; },
        .stop_song = [&] { ++invocations[5]; },
        .remove_song = [&] { ++invocations[6]; }
    });

    REQUIRE(registry.resolve("activate_itunes"));
    registry.resolve("activate_itunes")();

    for (const auto command : itunes::commands::all) {
        const auto action = registry.resolve(command.name);
        REQUIRE(action);
        action();
    }

    const std::array<int, 7> expected {1, 1, 1, 1, 1, 1, 1};
    CHECK(invocations == expected);
    CHECK_FALSE(registry.resolve("unknown_itunes_command"));
}
