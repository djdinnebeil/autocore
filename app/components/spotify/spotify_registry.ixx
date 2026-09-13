/**
 * \file spotify_registry.ixx
 * \brief Runtime command registry for `spotify_ac.exe`.
 */
export module spotify_registry;

import command_registry;

export struct spotify_command_actions {
    command_registry::Action get_queue;
    command_registry::Action print_songs;
    command_registry::Action play_pause;
    command_registry::Action next_song;
    command_registry::Action switch_player;
};

export command_registry::Registry create_spotify_command_registry(
    spotify_command_actions actions
);
export command_registry::Registry create_spotify_command_registry();
