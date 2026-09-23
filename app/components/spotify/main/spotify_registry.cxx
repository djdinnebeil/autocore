module spotify_registry;

import std;
import command_registry;
import spotify_protocol;

command_registry::Registry create_spotify_command_registry(
    spotify_command_actions actions
) {
    command_registry::Registry registry;
    registry.add(std::string {spotify::commands::get_queue.name}, std::move(actions.get_queue));
    registry.add(std::string {spotify::commands::print_songs.name}, std::move(actions.print_songs));
    registry.add(std::string {spotify::commands::play_pause.name}, std::move(actions.play_pause));
    registry.add(std::string {spotify::commands::next_song.name}, std::move(actions.next_song));
    registry.add(std::string {spotify::commands::switch_player.name}, std::move(actions.switch_player));
    return registry;
}
