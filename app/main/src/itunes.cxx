module itunes;

import std;
import pipes;
import journey;
import ac_component;

import <Windows.h>;

namespace {
    HANDLE ac_itunes_pipe = INVALID_HANDLE_VALUE;
}

void create_itunes_pipe() {
    ac_itunes_pipe = ac::pipes::create_pipe_server(L"ac_itunes_pipe", auto_core);
}

/**
 * \brief Starts the iTunes component.
 *
 * This function starts the iTunes component by creating a new process for the iTunes executable.
 */
void start_iTunes_component() {
    std::wstring itunes_path = LR"(.\ac_itunes.exe)";
    ac::main::create_process(itunes_path);
}

/**
 * \brief Sends a command to print iTunes songs.
 *
 * This function sends a command to the iTunes pipe to print the list of iTunes songs.
 *
 * \keymap_command
 */
void print_iTunes_songs() {
    ac::pipes::send_pipe_command(ac_itunes_pipe, 3);
}

/**
 * \brief Sends a command to skip to the next iTunes song.
 *
 * This function sends a command to the iTunes pipe to play the next song in iTunes.
 *
 * \keymap_command
 */
void iTunes_next_song() {
    ac::pipes::send_pipe_command(ac_itunes_pipe, 2);
}

/**
 * \brief Sends a command to print the next up song list.
 *
 * This function sends a command to the iTunes pipe to print the list of upcoming songs.
 *
 * \keymap_command
 */
void print_next_up_song_list() {
    ac::pipes::send_pipe_command(ac_itunes_pipe, 4);
}

/**
 * \brief Sends a command to play/pause iTunes.
 *
 * This function sends a command to the iTunes pipe to play or pause the current iTunes song.
 *
 * \keymap_command
 */
void iTunes_play_pause() {
    ac::pipes::send_pipe_command(ac_itunes_pipe, 1);
}

/**
 * \brief Sends a command to play the previous iTunes song.
 *
 * This function sends a command to the iTunes pipe to play the previous song in iTunes.
 */
void iTunes_prev_song() {
    ac::pipes::send_pipe_command(ac_itunes_pipe, 6);
}

/**
 * \brief Sends a command to stop the current next up.
 *
 * This function sends a command to the iTunes pipe to stop the current next up.
 *
 * \keymap_command
 */
void iTunes_stop_song() {
    ac::pipes::send_pipe_command(ac_itunes_pipe, 7);
}


/**
 * \brief Sends a command to update the iTunes logger.
 *
 * This function sends a command to the iTunes pipe to update the iTunes logger.
 */
void update_iTunes_logger() {
    ac::pipes::send_pipe_command(ac_itunes_pipe, 5);
}

/**
 * \brief Sends a command to stop the iTunes component.
 *
 * This function sends a command to the iTunes pipe to end the iTunes component.
 */
void send_iTunes_end_signal() {
    ac::pipes::send_pipe_command(ac_itunes_pipe, 0);
}

/**
 * \brief Sends a command to remove a song from iTunes.
 *
 * This function sends a command to the iTunes pipe to remove a song from iTunes.
 *
 * \keymap_command
 */
void remove_iTunes_song() {
    ac::pipes::send_pipe_command(ac_itunes_pipe, 9);
}
