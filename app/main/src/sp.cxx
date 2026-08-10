module sp;

import std;
import pipes;
import journey;
import ac_component;

import <Windows.h>;

namespace {
    HANDLE ac_sp_pipe = INVALID_HANDLE_VALUE;
}

void create_sp_pipe() {
    ac_sp_pipe = ac::pipes::create_pipe_server(L"ac_sp_pipe", auto_core);
}

/**
 * \brief Starts the Spotify component executable.
 *
 * This function launches the Spotify component executable, `sp_ac.exe`, which is responsible for handling Spotify-related tasks.
 */
void start_sp_component() {
    std::wstring sp_path = LR"(.\sp_ac.exe)";
    ac::main::create_process(sp_path);
}

/**
 * \brief Retrieves and displays the user's Spotify queue.
 *
 * This function sends a command to the Spotify component to fetch and display the user's current Spotify queue.
 * \keymap_command
 */
void get_user_sp_queue() {
    ac::pipes::send_pipe_command(ac_sp_pipe, 4);
}

/**
 * \brief Retrieves and displays the currently playing Spotify songs.
 *
 * This function sends a command to the Spotify component to fetch and display the currently playing Spotify songs.
 * \keymap_command
 */
void print_spotify_songs() {
    ac::pipes::send_pipe_command(ac_sp_pipe, 3);
}

/**
 * \brief Toggles play/pause on the Spotify player.
 *
 * This function sends a command to the Spotify component to toggle the play/pause state of the Spotify player.
 * \keymap_command
 */
void spotify_play_pause() {
    ac::pipes::send_pipe_command(ac_sp_pipe, 1);
}

/**
 * \brief Skips to the next song in the Spotify playlist.
 *
 * This function sends a command to the Spotify component to skip to the next song in the Spotify playlist.
 * \keymap_command
 */
void spotify_next_song() {
    ac::pipes::send_pipe_command(ac_sp_pipe, 2);
}

/**
 * \brief Switches the Spotify playback device.
 *
 * This function sends a command to the Spotify component to switch the playback device (e.g., between desktop and mobile).
 * \keymap_command
 */
void sp_switch_player() {
    ac::pipes::send_pipe_command(ac_sp_pipe, 6);
}

/**
 * \brief Downloads the album cover of the currently playing song.
 *
 * This function sends a command to the Spotify component to download the album cover of the currently playing song.
 */
void download_album_cover() {
    ac::pipes::send_pipe_command(ac_sp_pipe, 8);
}

/**
 * \brief Sends a signal to terminate the Spotify component.
 *
 * This function sends a command to the Spotify component to gracefully shut down and terminate the process.
 */
void send_sp_end_signal() {
    ac::pipes::send_pipe_command(ac_sp_pipe, 0);
}

/**
 * \brief Updates the Spotify logger.
 *
 * This function sends a command to the Spotify component to update the log file with the latest information.
 */
void update_sp_logger() {
    ac::pipes::send_pipe_command(ac_sp_pipe, 5);
}
