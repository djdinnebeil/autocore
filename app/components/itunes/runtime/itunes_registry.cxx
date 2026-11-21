module;
#include "../../../core/taskbar/activation_catalog_description.hpp"

module itunes_registry;
import std;
import command_registry;
import itunes_component;
import itunes_protocol;

command_registry::Registry create_itunes_command_registry(
    itunes_command_actions actions
) {
    command_registry::Registry registry;
    registry.add(
        "activate_itunes",
        std::move(actions.activate_itunes),
        ac::taskbar::activation_catalog_description(
            "activate_itunes",
            "itunes",
            {}
        )
    );
    registry.add(
        std::string {itunes::commands::print_songs.name},
        std::move(actions.print_songs),
        "Inserts the current iTunes track and collected song history using song.format."
    );
    registry.add(
        std::string {itunes::commands::next_song.name},
        std::move(actions.next_song),
        "Skips to the next iTunes track."
    );
    registry.add(
        std::string {itunes::commands::print_next_up.name},
        actions.print_next_up,
        "Formats clipboard lines with library.format and inserts them."
    );
    registry.add(
        "print_next_up_song_list",
        std::move(actions.print_next_up),
        "Formats clipboard lines with library.format and inserts them."
    );
    registry.add(
        std::string {itunes::commands::play_pause.name},
        std::move(actions.play_pause),
        "Plays or pauses iTunes."
    );
    registry.add(
        std::string {itunes::commands::stop_song.name},
        std::move(actions.stop_song),
        "Stops iTunes playback and inserts the current track."
    );
    registry.add(
        std::string {itunes::commands::remove_song.name},
        std::move(actions.remove_song),
        "Deletes the current iTunes track and moves its file to the Recycle Bin."
    );
    registry.add(
        "shared_command_provider",
        [] { itunes_component.print("Shared command in iTunes"); },
        "Shared command in iTunes"
    );
    return registry;
}
