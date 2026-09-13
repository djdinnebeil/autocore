module itunes_registry;
import std;
import command_registry;
import itunes_protocol;

command_registry::Registry create_itunes_command_registry(
    itunes_command_actions actions
) {
    command_registry::Registry registry;
    registry.add(std::string {itunes::commands::print_songs.name}, std::move(actions.print_songs));
    registry.add(std::string {itunes::commands::next_song.name}, std::move(actions.next_song));
    registry.add(std::string {itunes::commands::print_next_up.name}, std::move(actions.print_next_up));
    registry.add(std::string {itunes::commands::play_pause.name}, std::move(actions.play_pause));
    registry.add(std::string {itunes::commands::stop_song.name}, std::move(actions.stop_song));
    registry.add(std::string {itunes::commands::remove_song.name}, std::move(actions.remove_song));
    return registry;
}
