/**
\file main.cxx
\brief Entry point for the Spotify component of Auto Core.

This file initializes the Spotify component, sets up the command map for handling
various commands through named pipes, and manages the main loop for processing
pipe commands. It also handles logging initialization and shutdown.
 */
import std;
import spotify_component;
import spotify_client;
import spotify_monitor;
import auto_core.core.pipes;
import spotify_protocol;
import command_registry;
import spotify_registry;
import spotify_pipe;
import auto_core.taskbar;
import <Windows.h>;

/**
 * \brief Ends the Spotify process and performs necessary cleanup.
 */
void end_spotify() {
    spotify_component.logg("spotify_ac.exe is shutting down");
    stop_spotify_monitor();
}
/**
 * \brief Main function for the Spotify process.
 * Initializes logging, sets up the command map, starts the Spotify song thread,
 * and processes pipe commands.
 *
 * \return Exit code of the process.
 */
int main() {
    const auto registry = create_spotify_command_registry();
    int exit_code = 0;
    log_init();

    if (!ac::taskbar::connect()) {
        spotify_component.logg_and_print(
            "Unable to receive the native taskbar snapshot from Auto Core."
        );
    }
    struct TaskbarConnectionGuard {
        ~TaskbarConnectionGuard() { ac::taskbar::disconnect(); }
    } taskbar_connection_guard;

    start_spotify_monitor();

    ac::pipes::Pipe ac_spotify_pipe;
    auto connection = ac::pipes::connect_to_pipe_server(
        std::wstring { ac::protocol::spotify::pipe_name }
    );

    if (connection) {
        ac_spotify_pipe = std::move(*connection);
        ac::pipes::CommandDispatcher dispatcher;
        bool protocol_failed = false;
        register_spotify_pipe_commands(
            dispatcher,
            ac_spotify_pipe,
            registry,
            {
                .shutdown = &end_spotify,
                .play_pause = &spotify_play_pause,
                .next_song = &spotify_next_song,
                .print_songs = &spotify_print_songs,
                .get_queue = &spotify_get_queue,
                .update_component = &update_spotify_component,
                .switch_player = &spotify_switch_player,
                .download_album_cover = &spotify_download_album_cover,
                .unknown_named = [](const std::string_view name) {
                    spotify_component.logg_and_print("Unknown Spotify command: {}", name);
                }
            },
            protocol_failed
        );
        if (const auto result = dispatcher.process(ac_spotify_pipe);
            !result) {
            spotify_component.logg_and_print(
                "Spotify pipe failed. Error: {}",
                result.error().system_error
            );
        }
        if (protocol_failed) {
            exit_code = 1;
        }
    }
    else {
        spotify_component.logg_and_print(
            "Failed to connect to Spotify pipe. Error: {}",
            connection.error().system_error
        );
    }

    stop_spotify_monitor();

    spotify_component.logg_and_logg("spotify_ac.exe has ended");

    return exit_code;
}
