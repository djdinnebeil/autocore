// Spotify named-command registry tests.
#include "catch_amalgamated.hpp"

#include <array>
#include <string>
#include <vector>

import spotify_protocol;
import spotify_registry;

TEST_CASE("Spotify registry contains every canonical command", "[spotify][registry][unit]") {
    CHECK(spotify_activation_fallback_message ==
        "Pin or start Spotify, then run taskbar_builder.exe to generate "
        "spotify.map.");
    std::array<int, 6> invocations {};
    auto registry = create_spotify_command_registry({
        .activate_spotify = [&] { ++invocations[0]; },
        .get_queue = [&] { ++invocations[1]; },
        .print_songs = [&] { ++invocations[2]; },
        .play_pause = [&] { ++invocations[3]; },
        .next_song = [&] { ++invocations[4]; },
        .switch_player = [&] { ++invocations[5]; }
    });

    const std::vector<std::string> expected {
        "activate_spotify",
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
    std::array<int, 6> invocations {};
    auto registry = create_spotify_command_registry({
        .activate_spotify = [&] { ++invocations[0]; },
        .get_queue = [&] { ++invocations[1]; },
        .print_songs = [&] { ++invocations[2]; },
        .play_pause = [&] { ++invocations[3]; },
        .next_song = [&] { ++invocations[4]; },
        .switch_player = [&] { ++invocations[5]; }
    });

    REQUIRE(registry.resolve("activate_spotify"));
    registry.resolve("activate_spotify")();

    for (const auto command : spotify::commands::all) {
        const auto action = registry.resolve(command.name);
        REQUIRE(action);
        action();
    }

    const std::array<int, 6> expected {1, 1, 1, 1, 1, 1};
    CHECK(invocations == expected);
    CHECK_FALSE(registry.resolve("unknown_spotify_command"));
}
