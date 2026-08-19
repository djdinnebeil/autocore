/**
\file main.cxx
\brief Entry point for the Spotify component of Auto Core.

This file initializes the Spotify component, sets up the command map for handling
various commands through named pipes, and manages the main loop for processing
pipe commands. It also handles logging initialization and shutdown.
 */
import std;
import sp_x;
import sp_c;
import sp_t;
import auto_core.pipes;
import spotify_protocol;
import command_registry;
import sp_registry;
import <Windows.h>;

// Pipe name for Spotify communication
/**
 * \brief Ends the Spotify process and performs necessary cleanup.
 */
void end_sp() {
    sp_component.logg("sp_ac.exe is shutting down");
    ac_spotify.end_thread = true;
    sp_playback_state_change = true;
    sp_cv.notify_one();
}
/**
 * \brief Sets up the command map for pipe communication.
 */
void set_commands(ac::pipes::CommandDispatcher& dispatcher, ac::pipes::Pipe& pipe,
    const command_registry::Registry& registry, bool& protocol_failed) {
    dispatcher.set_command(ac::protocol::spotify::to_wire(ac::protocol::spotify::Command::shutdown), [&dispatcher]() {
        end_sp();
        dispatcher.request_stop();
    });
    dispatcher.set_command(ac::protocol::spotify::to_wire(ac::protocol::spotify::Command::play_pause), spotify_play_pause);
    dispatcher.set_command(ac::protocol::spotify::to_wire(ac::protocol::spotify::Command::next_song), spotify_next_song);
    dispatcher.set_command(ac::protocol::spotify::to_wire(ac::protocol::spotify::Command::print_songs), print_spotify_songs);
    dispatcher.set_command(ac::protocol::spotify::to_wire(ac::protocol::spotify::Command::get_queue), get_user_sp_queue);
    dispatcher.set_command(ac::protocol::spotify::to_wire(ac::protocol::spotify::Command::update_component), update_sp_component);
    dispatcher.set_command(ac::protocol::spotify::to_wire(ac::protocol::spotify::Command::switch_player), sp_switch_player);
    dispatcher.set_command(ac::protocol::spotify::to_wire(ac::protocol::spotify::Command::download_album_cover), download_album_cover);
    dispatcher.set_command(ac::protocol::spotify::to_wire(ac::protocol::spotify::Command::invoke_named),
        [&dispatcher, &pipe, &registry, &protocol_failed] {
            const auto name = ac::pipes::read_string(pipe);
            if (!name) { protocol_failed = true; dispatcher.request_stop(); return; }
            if (auto action = registry.resolve(*name)) action();
            else sp_component.logg_and_print("Unknown Spotify command: {}", *name);
        });
}

/**
 * \brief Main function for the Spotify process.
 * Initializes logging, sets up the command map, starts the Spotify song thread,
 * and processes pipe commands.
 *
 * \return Exit code of the process.
 */
int main(const int argument_count, char* arguments[]) {
    log_init();
    const auto registry = create_sp_command_registry();

    for (int index = 1; index + 1 < argument_count; ++index) {
        if (std::string_view {arguments[index]} !=
            "--taskbar-position") {
            continue;
        }

        int position = -1;
        const std::string_view value {arguments[index + 1]};
        const auto result = std::from_chars(
            value.data(), value.data() + value.size(), position
        );

        if (result.ec == std::errc {} &&
            result.ptr == value.data() + value.size()) {
            ac_spotify.set_taskbar_position(position);
        }
        break;
    }

    start_sp_song_thread();

    ac::pipes::Pipe ac_sp_pipe;
    auto connection = ac::pipes::connect_to_pipe_server(
        std::wstring { ac::protocol::spotify::pipe_name }
    );

    if (connection) {
        ac_sp_pipe = std::move(*connection);
        ac::pipes::CommandDispatcher dispatcher;
        bool protocol_failed = false;
        set_commands(dispatcher, ac_sp_pipe, registry, protocol_failed);
        if (const auto result = dispatcher.process(ac_sp_pipe);
            !result) {
            sp_component.logg_and_print(
                "Spotify pipe failed. Error: {}",
                result.error().system_error
            );
        }
        if (protocol_failed) return 1;
    }
    else {
        sp_component.logg_and_print(
            "Failed to connect to Spotify pipe. Error: {}",
            connection.error().system_error
        );
    }

    sp_component.logg_and_logg("sp_ac.exe has ended");

    return 0;
}
