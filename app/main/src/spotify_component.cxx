module auto_core.main.components.spotify;

import std;
import auto_core.core.pipes;
import auto_core.main.application;
import auto_core.core.paths;
import spotify_protocol;

import <Windows.h>;

namespace {
    ac::pipes::Pipe ac_spotify_pipe;
    std::mutex spotify_pipe_mutex;

    void invoke_named(std::string_view name) {
        const std::scoped_lock lock {spotify_pipe_mutex};
        if (const auto header = ac::pipes::send_pipe_command(ac_spotify_pipe,
                ac::protocol::spotify::to_wire(ac::protocol::spotify::Command::invoke_named)); !header) return;
        if (const auto payload = ac::pipes::send_string(ac_spotify_pipe, name); !payload) {
            auto_core.logg_and_print("Failed to send Spotify command. Error: {}", payload.error().system_error);
        }
    }

    void send_command(ac::protocol::spotify::Command command) {
        const std::scoped_lock lock {spotify_pipe_mutex};
        if (const auto result = ac::pipes::send_pipe_command(
                ac_spotify_pipe,
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

void create_spotify_pipe() {
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

    ac_spotify_pipe = std::move(*result);
}

/**
 * \brief Starts the Spotify component executable.
 *
 * This function launches the Spotify component executable, `spotify_ac.exe`, which is responsible for handling Spotify-related tasks.
 */
void start_spotify_component() {
    const std::filesystem::path spotify_path =
        ac::paths::executable_directory() / "spotify_ac.exe";

    ac::main::create_process(spotify_path);
}

/**
 * \brief Sends a signal to terminate the Spotify component.
 *
 * This function sends a command to the Spotify component to gracefully shut down and terminate the process.
 */
void send_spotify_end_signal() {
    send_command(ac::protocol::spotify::Command::shutdown);
}

void spotify_component::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    std::unordered_set<std::string> names;
    std::ifstream input(ac::paths::keymap_components_directory() / ac::protocol::spotify::manifest_filename);
    std::string name;
    while (std::getline(input, name)) {
        if (!name.empty() && name.back() == '\r') name.pop_back();
        if (name.starts_with("\xEF\xBB\xBF")) name.erase(0, 3);
        if (!name.empty()) names.insert(name);
    }
    if (!input.is_open()) for (const auto command : spotify::commands::all) names.emplace(command.name);
    for (const auto& value : names) registry.add(value, [value] { invoke_named(value); });
}
