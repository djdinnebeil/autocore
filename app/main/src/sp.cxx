module sp;

import std;
import auto_core.pipes;
import journey;
import ac_main;
import auto_core.paths;
import spotify_protocol;
import taskbar;

import <Windows.h>;

namespace {
    ac::pipes::Pipe ac_sp_pipe;

    void send_command(ac::pipes::Pipe& pipe, ac::protocol::spotify::Command command) {
        if (const auto result = ac::pipes::send_pipe_command(
                pipe,
                ac::protocol::spotify::to_wire(command)
            );
            !result) {
            auto_core.logg_and_print(
                "Failed to send Spotify command. Error: {}",
                result.error().system_error
            );
        }
    }
}

void create_sp_pipe() {
    auto result = ac::pipes::create_pipe_server(
        std::wstring { ac::protocol::spotify::pipe_name }
    );
    if (!result) {
        auto_core.logg_and_print(
            "Failed to create Spotify pipe. Error: {}",
            result.error().system_error
        );
        return;
    }

    ac_sp_pipe = std::move(*result);
}

/**
 * \brief Starts the Spotify component executable.
 *
 * This function launches the Spotify component executable, `sp_ac.exe`, which is responsible for handling Spotify-related tasks.
 */
void start_sp_component() {
    const std::filesystem::path sp_path =
        ac::paths::executable_directory() / "sp_ac.exe";

    std::wstring arguments;

    if (const auto position = taskbar_position("spotify")) {
        arguments = std::format(
            L"--taskbar-position {}",
            *position
        );
    }
    else {
        auto_core.logg_and_print(
            "Spotify taskbar position is not configured."
        );
    }

    ac::main::create_process(sp_path, arguments);
}

/**
 * \brief Retrieves and displays the user's Spotify queue.
 *
 * This function sends a command to the Spotify component to fetch and display the user's current Spotify queue.
 * \keymap_command
 */
void get_user_sp_queue() {
    send_command(ac_sp_pipe, ac::protocol::spotify::Command::get_queue);
}

/**
 * \brief Retrieves and displays the currently playing Spotify songs.
 *
 * This function sends a command to the Spotify component to fetch and display the currently playing Spotify songs.
 * \keymap_command
 */
void print_spotify_songs() {
    send_command(ac_sp_pipe, ac::protocol::spotify::Command::print_songs);
}

/**
 * \brief Toggles play/pause on the Spotify player.
 *
 * This function sends a command to the Spotify component to toggle the play/pause state of the Spotify player.
 * \keymap_command
 */
void spotify_play_pause() {
    send_command(ac_sp_pipe, ac::protocol::spotify::Command::play_pause);
}

/**
 * \brief Skips to the next song in the Spotify playlist.
 *
 * This function sends a command to the Spotify component to skip to the next song in the Spotify playlist.
 * \keymap_command
 */
void spotify_next_song() {
    send_command(ac_sp_pipe, ac::protocol::spotify::Command::next_song);
}

/**
 * \brief Switches the Spotify playback device.
 *
 * This function sends a command to the Spotify component to switch the playback device (e.g., between desktop and mobile).
 * \keymap_command
 */
void sp_switch_player() {
    send_command(ac_sp_pipe, ac::protocol::spotify::Command::switch_player);
}

/**
 * \brief Downloads the album cover of the currently playing song.
 *
 * This function sends a command to the Spotify component to download the album cover of the currently playing song.
 */
void download_album_cover() {
    send_command(ac_sp_pipe, ac::protocol::spotify::Command::download_album_cover);
}

/**
 * \brief Sends a signal to terminate the Spotify component.
 *
 * This function sends a command to the Spotify component to gracefully shut down and terminate the process.
 */
void send_sp_end_signal() {
    send_command(ac_sp_pipe, ac::protocol::spotify::Command::shutdown);
}

/**
 * \brief Updates the Spotify logger.
 *
 * This function sends a command to the Spotify component to update the log file with the latest information.
 */
void update_sp_logger() {
    send_command(ac_sp_pipe, ac::protocol::spotify::Command::update_component);
}

void sp::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("get_user_sp_queue", &::get_user_sp_queue);
    registry.add("print_spotify_songs", &::print_spotify_songs);
    registry.add("spotify_play_pause", &::spotify_play_pause);
    registry.add("spotify_next_song", &::spotify_next_song);
    registry.add("sp_switch_player", &::sp_switch_player);
}
