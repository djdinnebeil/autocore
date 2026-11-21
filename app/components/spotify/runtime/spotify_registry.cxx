module;
#include "../../../core/taskbar/activation_catalog_description.hpp"

module spotify_registry;

import std;
import command_registry;
import spotify_component;
import spotify_protocol;

command_registry::Registry create_spotify_command_registry(
    spotify_command_actions actions
) {
    command_registry::Registry registry;
    registry.add(
        "activate_spotify",
        std::move(actions.activate_spotify),
        ac::taskbar::activation_catalog_description(
            "activate_spotify",
            "spotify",
            {}
        )
    );
    registry.add(
        std::string {spotify::commands::get_queue.name},
        std::move(actions.get_queue),
        "Inserts the Spotify queue and current track using song.format."
    );
    registry.add(
        std::string {spotify::commands::print_songs.name},
        std::move(actions.print_songs),
        "Inserts the collected Spotify song history and the current track using song.format."
    );
    registry.add(
        std::string {spotify::commands::play_pause.name},
        std::move(actions.play_pause),
        "Pauses Spotify when it is playing; otherwise starts playback."
    );
    registry.add(
        std::string {spotify::commands::next_song.name},
        std::move(actions.next_song),
        "Skips to the next Spotify track."
    );
    registry.add(
        std::string {spotify::commands::switch_player.name},
        std::move(actions.switch_player),
        "Moves Spotify playback to the next configured device."
    );
    registry.add(
        "shared_command_provider",
        [] { spotify_component.print("Shared command in Spotify"); },
        "Shared command in Spotify"
    );
    return registry;
}
