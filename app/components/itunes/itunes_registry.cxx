module itunes_registry;
import command_registry;
import itunes_protocol;
import itunes_c;
import itunes_t;
import slash_i;

command_registry::Registry create_itunes_command_registry() {
    command_registry::Registry registry;
    registry.add(std::string {itunes::commands::print_songs.name}, &print_iTunes_songs);
    registry.add(std::string {itunes::commands::next_song.name}, &iTunes_next_song);
    registry.add(std::string {itunes::commands::print_next_up.name}, &print_next_up_song_list);
    registry.add(std::string {itunes::commands::play_pause.name}, &iTunes_play_pause);
    registry.add(std::string {itunes::commands::stop_song.name}, &iTunes_stop_song);
    registry.add(std::string {itunes::commands::remove_song.name}, &remove_iTunes_song);
    return registry;
}
