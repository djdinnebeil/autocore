/**
 * \file itunes_pipe.ixx
 * \brief Connects the iTunes wire protocol to injectable command actions.
 */
export module itunes_pipe;

import std;

import auto_core.core.pipes;
import command_registry;

/** Actions invoked for each iTunes pipe command. */
export struct itunes_pipe_actions {
    command_registry::Action shutdown;
    command_registry::Action play_pause;
    command_registry::Action next_song;
    command_registry::Action print_songs;
    command_registry::Action print_next_up;
    command_registry::Action update_component;
    command_registry::Action previous_song;
    command_registry::Action stop_song;
    command_registry::Action remove_song;
    std::function<void(std::string_view)> unknown_named;
};

/** Binds protocol integers and named invokes to `actions`. */
export void register_itunes_pipe_commands(
    ac::pipes::CommandDispatcher& dispatcher,
    ac::pipes::Pipe& pipe,
    const command_registry::Registry& registry,
    itunes_pipe_actions actions,
    bool& protocol_failed
);
