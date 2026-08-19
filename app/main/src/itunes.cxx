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
    std::mutex itunes_pipe_mutex;

    void invoke_named(std::string_view name) {
        const std::scoped_lock lock {itunes_pipe_mutex};
        if (const auto header = ac::pipes::send_pipe_command(ac_itunes_pipe,
                ac::protocol::itunes::to_wire(ac::protocol::itunes::Command::invoke_named)); !header) return;
        if (const auto payload = ac::pipes::send_string(ac_itunes_pipe, name); !payload) {
            auto_core.logg_and_print("Failed to send iTunes command. Error: {}", payload.error().system_error);
        }
    }

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
    std::unordered_set<std::string> names;
    std::ifstream input(ac::paths::executable_directory() / ac::protocol::itunes::manifest_filename);
    std::string name;
    while (std::getline(input, name)) {
        if (!name.empty() && name.back() == '\r') name.pop_back();
        if (name.starts_with("\xEF\xBB\xBF")) name.erase(0, 3);
        if (!name.empty()) names.insert(name);
    }
    if (!input.is_open()) for (const auto command : itunes::commands::all) names.emplace(command.name);
    for (const auto& value : names) registry.add(value, [value] { invoke_named(value); });
    registry.add("print_iTunes_songs", [] { invoke_named(itunes::commands::print_songs.name); });
    registry.add("iTunes_next_song", [] { invoke_named(itunes::commands::next_song.name); });
    registry.add("print_next_up_song_list", [] { invoke_named(itunes::commands::print_next_up.name); });
    registry.add("iTunes_play_pause", [] { invoke_named(itunes::commands::play_pause.name); });
    registry.add("iTunes_stop_song", [] { invoke_named(itunes::commands::stop_song.name); });
    registry.add("remove_iTunes_song", [] { invoke_named(itunes::commands::remove_song.name); });
}

std::function<void()> itunes_command(ac::protocol::itunes::CommandName command) {
    return [name = std::string {command.name}] { invoke_named(name); };
}
