module itunes_registry;

import itunes_client;
import itunes_monitor;
import itunes_removal;

command_registry::Registry create_itunes_command_registry() {
    return create_itunes_command_registry({
        .print_songs = &print_itunes_songs,
        .next_song = &itunes_next_song,
        .print_next_up = &print_next_up_song_list,
        .play_pause = &itunes_play_pause,
        .stop_song = &itunes_stop_song,
        .remove_song = &remove_itunes_song
    });
}
