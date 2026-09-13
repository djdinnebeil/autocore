// Spotify named-command registry tests.
#include "catch_amalgamated.hpp"

#include <array>
#include <string>
#include <vector>

import spotify_protocol;
import spotify_registry;

TEST_CASE("Spotify registry contains every canonical command", "[spotify][registry][unit]") {
    std::array<int, 5> invocations {};
    auto registry = create_spotify_command_registry({
        .get_queue = [&] { ++invocations[0]; },
        .print_songs = [&] { ++invocations[1]; },
        .play_pause = [&] { ++invocations[2]; },
        .next_song = [&] { ++invocations[3]; },
        .switch_player = [&] { ++invocations[4]; }
    });

    const std::vector<std::string> expected {
        "spotify_get_queue",
        "spotify_next_song",
        "spotify_play_pause",
        "spotify_print_songs",
        "spotify_switch_player"
    };

    CHECK(registry.registered_names() == expected);
    for (const auto command : spotify::commands::all) {
        CHECK(registry.contains(command.name));
    }
    CHECK_FALSE(registry.contains("print_spotify_songs"));
    CHECK_FALSE(registry.contains("get_user_sp_queue"));
    CHECK_FALSE(registry.contains("sp_play_pause"));
    CHECK_FALSE(registry.contains("sp_next_song"));
    CHECK_FALSE(registry.contains("unknown_spotify_command"));
}

TEST_CASE("Spotify registry dispatches each command to its action", "[spotify][registry][unit]") {
    std::array<int, 5> invocations {};
    auto registry = create_spotify_command_registry({
        .get_queue = [&] { ++invocations[0]; },
        .print_songs = [&] { ++invocations[1]; },
        .play_pause = [&] { ++invocations[2]; },
        .next_song = [&] { ++invocations[3]; },
        .switch_player = [&] { ++invocations[4]; }
    });

    for (const auto command : spotify::commands::all) {
        const auto action = registry.resolve(command.name);
        REQUIRE(action);
        action();
    }

    const std::array<int, 5> expected {1, 1, 1, 1, 1};
    CHECK(invocations == expected);
    CHECK_FALSE(registry.resolve("unknown_spotify_command"));
}
