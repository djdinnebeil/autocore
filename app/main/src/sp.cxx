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
    std::mutex sp_pipe_mutex;

    void invoke_named(std::string_view name) {
        const std::scoped_lock lock {sp_pipe_mutex};
        if (const auto header = ac::pipes::send_pipe_command(ac_sp_pipe,
                ac::protocol::spotify::to_wire(ac::protocol::spotify::Command::invoke_named)); !header) return;
        if (const auto payload = ac::pipes::send_string(ac_sp_pipe, name); !payload) {
            auto_core.logg_and_print("Failed to send Spotify command. Error: {}", payload.error().system_error);
        }
    }

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
    std::unordered_set<std::string> names;
    std::ifstream input(ac::paths::executable_directory() / ac::protocol::spotify::manifest_filename);
    std::string name;
    while (std::getline(input, name)) {
        if (!name.empty() && name.back() == '\r') name.pop_back();
        if (name.starts_with("\xEF\xBB\xBF")) name.erase(0, 3);
        if (!name.empty()) names.insert(name);
    }
    if (!input.is_open()) for (const auto command : sp::commands::all) names.emplace(command.name);
    for (const auto& value : names) registry.add(value, [value] { invoke_named(value); });
    registry.add("get_user_sp_queue", [] { invoke_named(sp::commands::get_queue.name); });
    registry.add("print_spotify_songs", [] { invoke_named(sp::commands::print_songs.name); });
    registry.add("spotify_play_pause", [] { invoke_named(sp::commands::play_pause.name); });
    registry.add("spotify_next_song", [] { invoke_named(sp::commands::next_song.name); });
}

std::function<void()> sp_command(ac::protocol::spotify::CommandName command) {
    return [name = std::string {command.name}] { invoke_named(name); };
}
