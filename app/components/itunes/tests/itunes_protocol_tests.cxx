#include "catch_amalgamated.hpp"

#include <set>
#include <string_view>

import itunes_protocol;

TEST_CASE("iTunes command wire values remain stable", "[itunes][protocol][unit]") {
    using ac::protocol::itunes::Command;
    using ac::protocol::itunes::to_wire;

    CHECK(to_wire(Command::shutdown) == 0);
    CHECK(to_wire(Command::play_pause) == 1);
    CHECK(to_wire(Command::next_song) == 2);
    CHECK(to_wire(Command::print_songs) == 3);
    CHECK(to_wire(Command::print_next_up) == 4);
    CHECK(to_wire(Command::update_component) == 5);
    CHECK(to_wire(Command::previous_song) == 6);
    CHECK(to_wire(Command::stop_song) == 7);
    CHECK(to_wire(Command::remove_song) == 9);
    CHECK(to_wire(Command::invoke_named) == 10);
}

TEST_CASE("iTunes canonical command names are unique", "[itunes][protocol][unit]") {
    std::set<std::string_view> names;

    for (const auto command : itunes::commands::all) {
        CHECK(command.name.starts_with("itunes_"));
        CHECK(names.insert(command.name).second);
    }

    CHECK(names.size() == itunes::commands::all.size());
}

TEST_CASE("iTunes protocol exposes stable resource names", "[itunes][protocol][unit]") {
    CHECK(ac::protocol::itunes::pipe_name == L"ac_itunes_pipe");
    CHECK(
        ac::protocol::itunes::manifest_filename ==
        "itunes.keymap_commands.txt"
    );
}
