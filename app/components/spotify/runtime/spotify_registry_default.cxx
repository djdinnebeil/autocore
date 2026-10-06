module spotify_registry;

import spotify_client;
import spotify_monitor;

command_registry::Registry create_spotify_command_registry() {
    return create_spotify_command_registry({
        .get_queue = &spotify_get_queue,
        .print_songs = &spotify_print_songs,
        .play_pause = &spotify_play_pause,
        .next_song = &spotify_next_song,
        .switch_player = &spotify_switch_player
    });
}
