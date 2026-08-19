/**
 * \file itunes.ixx
 * \brief This module provides support for music player integration with iTunes.
 *
 * This module enables integration with the iTunes music player, allowing control
 * and interaction through various commands. It includes functions for starting
 * the iTunes component, playing/pausing music, navigating songs, and updating logs.
 */
export module itunes;

export import itunes_protocol;
export import command_registry;
import std;

export namespace itunes::runtime_commands {
    void register_with(command_registry::Registry& registry);
}

export {
    void create_itunes_pipe();
    void start_iTunes_component();
    void print_iTunes_songs();
    void print_next_up_song_list();
    void iTunes_play_pause();
    void iTunes_prev_song();
    void iTunes_next_song();
    void iTunes_stop_song();
    void update_iTunes_logger();
    void send_iTunes_end_signal();
    void remove_iTunes_song();
    std::function<void()> itunes_command(ac::protocol::itunes::CommandName command);
}
