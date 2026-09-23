/**
 * \file itunes_registry.ixx
 * \brief Runtime command registry for `itunes_ac.exe`.
 */
export module itunes_registry;
import command_registry;

export struct itunes_command_actions {
    command_registry::Action print_songs;
    command_registry::Action next_song;
    command_registry::Action print_next_up;
    command_registry::Action play_pause;
    command_registry::Action stop_song;
    command_registry::Action remove_song;
};

export command_registry::Registry create_itunes_command_registry(
    itunes_command_actions actions
);
export command_registry::Registry create_itunes_command_registry();
