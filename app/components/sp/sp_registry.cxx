module sp_registry;

import command_registry;
import spotify_protocol;
import sp_c;
import sp_t;

command_registry::Registry create_sp_command_registry() {
    command_registry::Registry registry;
    registry.add(std::string {sp::commands::get_queue.name}, &get_user_sp_queue);
    registry.add(std::string {sp::commands::print_songs.name}, &print_spotify_songs);
    registry.add(std::string {sp::commands::play_pause.name}, &spotify_play_pause);
    registry.add(std::string {sp::commands::next_song.name}, &spotify_next_song);
    registry.add(std::string {sp::commands::switch_player.name}, &sp_switch_player);
    return registry;
}
