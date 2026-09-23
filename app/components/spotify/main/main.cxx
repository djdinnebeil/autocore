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
import component_protocol;
import auto_core.core.ini;
import auto_core.core.paths;
import <Windows.h>;

/**
 * \brief Ends the Spotify process and performs necessary cleanup.
 */
void end_spotify() {
    spotify_component.log_and_log("shutdown signal received");
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

    {
        const auto ini_path = ac::paths::config_directory() / "spotify.ini";
        if (!ac::ini::read(ini_path)) {
            std::error_code exists_error;
            const bool present =
                std::filesystem::exists(ini_path, exists_error);
            spotify_component.report_ini_unavailable(present && !exists_error);
        }
    }

    if (!ac::taskbar::connect()) {
        spotify_component.log_and_print(
            "Unable to receive the native taskbar snapshot from Auto Core."
        );
    }
    struct TaskbarConnectionGuard {
        ~TaskbarConnectionGuard() { ac::taskbar::disconnect(); }
    } taskbar_connection_guard;

    start_spotify_monitor();

    ac::pipes::Pipe ac_spotify_pipe;
    auto connection = ac::pipes::connect_to_pipe_server(
        ac::protocol::component::pipe_name("spotify")
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
                    spotify_component.log_and_print("Unknown Spotify command: {}", name);
                }
            },
            protocol_failed
        );
        if (const auto hello = ac::pipes::send_string(
                ac_spotify_pipe,
                ac::protocol::component::make_hello(
                    registry.autocomplete_values()
                )
            ); !hello) {
            spotify_component.log_and_print(
                "Failed to send Spotify hello. Error: {}",
                hello.error().system_error
            );
            exit_code = 1;
        }
        else if (const auto result = dispatcher.process(ac_spotify_pipe);
            !result) {
            spotify_component.log_and_print(
                "Spotify pipe failed. Error: {}",
                result.error().system_error
            );
        }
        if (protocol_failed) {
            exit_code = 1;
        }
    }
    else {
        spotify_component.log_and_print(
            "Failed to connect to Spotify pipe. Error: {}",
            connection.error().system_error
        );
    }

    stop_spotify_monitor();

    spotify_component.log_and_log("program terminated");

    return exit_code;
}
