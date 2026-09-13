module auto_core.main.components.itunes;

import std;
import auto_core.core.pipes;
import auto_core.main.application;
import auto_core.core.paths;
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

    void send_command(ac::protocol::itunes::Command command) {
        if (const auto result = ac::pipes::send_pipe_command(
                ac_itunes_pipe,
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
void start_itunes_component() {
    const std::filesystem::path itunes_path =
        ac::paths::executable_directory() / "itunes_ac.exe";

    ac::main::create_process(itunes_path);
}

/**
 * \brief Sends a command to stop the iTunes component.
 *
 * This function sends a command to the iTunes pipe to end the iTunes component.
 */
void send_itunes_end_signal() {
    send_command(ac::protocol::itunes::Command::shutdown);
}

void itunes_component::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    std::unordered_set<std::string> names;
    std::ifstream input(ac::paths::keymap_components_directory() / ac::protocol::itunes::manifest_filename);
    std::string name;
    while (std::getline(input, name)) {
        if (!name.empty() && name.back() == '\r') name.pop_back();
        if (name.starts_with("\xEF\xBB\xBF")) name.erase(0, 3);
        if (!name.empty()) names.insert(name);
    }
    if (!input.is_open()) for (const auto command : itunes::commands::all) names.emplace(command.name);
    for (const auto& value : names) registry.add(value, [value] { invoke_named(value); });
    registry.add("print_next_up_song_list", [] { invoke_named(itunes::commands::print_next_up.name); });
}
