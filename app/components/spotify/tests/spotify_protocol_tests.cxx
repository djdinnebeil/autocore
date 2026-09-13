// Spotify wire-contract characterization tests.
#include "catch_amalgamated.hpp"

#include <set>
#include <string_view>

import spotify_protocol;

TEST_CASE("Spotify command wire values remain stable", "[spotify][protocol][unit]") {
    using ac::protocol::spotify::Command;
    using ac::protocol::spotify::to_wire;

    CHECK(to_wire(Command::shutdown) == 0);
    CHECK(to_wire(Command::play_pause) == 1);
    CHECK(to_wire(Command::next_song) == 2);
    CHECK(to_wire(Command::print_songs) == 3);
    CHECK(to_wire(Command::get_queue) == 4);
    CHECK(to_wire(Command::update_component) == 5);
    CHECK(to_wire(Command::switch_player) == 6);
    CHECK(to_wire(Command::download_album_cover) == 8);
    CHECK(to_wire(Command::invoke_named) == 9);
}

TEST_CASE("Spotify canonical command names are unique", "[spotify][protocol][unit]") {
    std::set<std::string_view> names;

    for (const auto command : spotify::commands::all) {
        CHECK(command.name.starts_with("spotify_"));
        CHECK(names.insert(command.name).second);
    }

    CHECK(names.size() == spotify::commands::all.size());
}

TEST_CASE("Spotify protocol exposes stable resource names", "[spotify][protocol][unit]") {
    CHECK(ac::protocol::spotify::pipe_name == L"ac_spotify_pipe");
    CHECK(
        ac::protocol::spotify::manifest_filename ==
        "spotify.keymap_commands.txt"
    );
}
