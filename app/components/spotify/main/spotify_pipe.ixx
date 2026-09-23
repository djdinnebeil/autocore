/**
 * \file spotify_pipe.ixx
 * \brief Connects the Spotify wire protocol to injectable command actions.
 */
export module spotify_pipe;

import std;

import auto_core.core.pipes;
import command_registry;

/** Actions invoked for each Spotify pipe command, including album-cover download. */
export struct spotify_pipe_actions {
    command_registry::Action shutdown;
    command_registry::Action play_pause;
    command_registry::Action next_song;
    command_registry::Action print_songs;
    command_registry::Action get_queue;
    command_registry::Action update_component;
    command_registry::Action switch_player;
    command_registry::Action download_album_cover;
    std::function<void(std::string_view)> unknown_named;
};

/** Binds protocol integers and named invokes to `actions`. */
export void register_spotify_pipe_commands(
    ac::pipes::CommandDispatcher& dispatcher,
    ac::pipes::Pipe& pipe,
    const command_registry::Registry& registry,
    spotify_pipe_actions actions,
    bool& protocol_failed
);
