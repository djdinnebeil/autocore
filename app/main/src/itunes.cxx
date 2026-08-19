module itunes;

import std;
import auto_core.pipes;
import journey;
import ac_main;
import auto_core.paths;
import itunes_protocol;

import <Windows.h>;

namespace {
    ac::pipes::Pipe ac_itunes_pipe;

    void send_command(ac::pipes::Pipe& pipe, ac::protocol::itunes::Command command) {
        if (const auto result = ac::pipes::send_pipe_command(
                pipe,
                ac::protocol::itunes::to_wire(command)
            );
            !result) {
            auto_core.logg_and_print(
                "Failed to send iTunes command. Error: {}",
                result.error().system_error
            );
        }
    }
}

void create_itunes_pipe() {
    auto result = ac::pipes::create_pipe_server(
        std::wstring { ac::protocol::itunes::pipe_name }
    );
    if (!result) {
        auto_core.logg_and_print(
            "Failed to create iTunes pipe. Error: {}",
            result.error().system_error
        );
        return;
    }

    ac_itunes_pipe = std::move(*result);
}

/**
 * \brief Starts the iTunes component.
 *
 * This function starts the iTunes component by creating a new process for the iTunes executable.
 */
void start_iTunes_component() {
    const std::filesystem::path itunes_path =
        ac::paths::executable_directory() / "ac_itunes.exe";

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
    send_command(ac_itunes_pipe, ac::protocol::itunes::Command::print_songs);
}

/**
 * \brief Sends a command to skip to the next iTunes song.
 *
 * This function sends a command to the iTunes pipe to play the next song in iTunes.
 *
 * \keymap_command
 */
void iTunes_next_song() {
    send_command(ac_itunes_pipe, ac::protocol::itunes::Command::next_song);
}

/**
 * \brief Sends a command to print the next up song list.
 *
 * This function sends a command to the iTunes pipe to print the list of upcoming songs.
 *
 * \keymap_command
 */
void print_next_up_song_list() {
    send_command(ac_itunes_pipe, ac::protocol::itunes::Command::print_next_up);
}

/**
 * \brief Sends a command to play/pause iTunes.
 *
 * This function sends a command to the iTunes pipe to play or pause the current iTunes song.
 *
 * \keymap_command
 */
void iTunes_play_pause() {
    send_command(ac_itunes_pipe, ac::protocol::itunes::Command::play_pause);
}

/**
 * \brief Sends a command to play the previous iTunes song.
 *
 * This function sends a command to the iTunes pipe to play the previous song in iTunes.
 */
void iTunes_prev_song() {
    send_command(ac_itunes_pipe, ac::protocol::itunes::Command::previous_song);
}

/**
 * \brief Sends a command to stop the current next up.
 *
 * This function sends a command to the iTunes pipe to stop the current next up.
 *
 * \keymap_command
 */
void iTunes_stop_song() {
    send_command(ac_itunes_pipe, ac::protocol::itunes::Command::stop_song);
}


/**
 * \brief Sends a command to update the iTunes logger.
 *
 * This function sends a command to the iTunes pipe to update the iTunes logger.
 */
void update_iTunes_logger() {
    send_command(ac_itunes_pipe, ac::protocol::itunes::Command::update_component);
}

/**
 * \brief Sends a command to stop the iTunes component.
 *
 * This function sends a command to the iTunes pipe to end the iTunes component.
 */
void send_iTunes_end_signal() {
    send_command(ac_itunes_pipe, ac::protocol::itunes::Command::shutdown);
}

/**
 * \brief Sends a command to remove a song from iTunes.
 *
 * This function sends a command to the iTunes pipe to remove a song from iTunes.
 *
 * \keymap_command
 */
void remove_iTunes_song() {
    send_command(ac_itunes_pipe, ac::protocol::itunes::Command::remove_song);
}

void itunes::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("print_iTunes_songs", &::print_iTunes_songs);
    registry.add("iTunes_next_song", &::iTunes_next_song);
    registry.add("print_next_up_song_list", &::print_next_up_song_list);
    registry.add("iTunes_play_pause", &::iTunes_play_pause);
    registry.add("iTunes_stop_song", &::iTunes_stop_song);
    registry.add("remove_iTunes_song", &::remove_iTunes_song);
}
