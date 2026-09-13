module auto_core.main.components.wake;

import std;
import auto_core.core.pipes;
import auto_core.main.application;
import auto_core.core.paths;
import wake_protocol;
import <Windows.h>;

namespace {
    ac::pipes::Pipe wake_pipe;

    void send_command(ac::pipes::Pipe& pipe, ac::protocol::wake::Command command) {
        if (const auto result = ac::pipes::send_pipe_command(
                pipe,
                ac::protocol::wake::to_wire(command)
            );
            !result) {
            auto_core.logg_and_print(
                "Failed to send wake command. Error: {}",
                result.error().system_error
            );
        }
    }
}

void create_wake_pipe() {
    auto result = ac::pipes::create_pipe_server(
        std::wstring { ac::protocol::wake::pipe_name }
    );
    if (!result) {
        auto_core.logg_and_print(
            "Failed to create wake pipe. Error: {}",
            result.error().system_error
        );
        return;
    }

    wake_pipe = std::move(*result);
}

/**
 * \brief Starts the wake component executable.
 *
 * This function launches the wake component executable, `wake_ac.exe`, which is responsible for logging wake events.
 */
void start_wake_component() {
    const std::filesystem::path wake_path =
        ac::paths::executable_directory() / "wake_ac.exe";

    ac::main::create_process(wake_path);
}

/**
 * \brief Sends a signal to terminate the wake component.
 *
 * This function sends a command to the wake component to gracefully shut down and terminate the process.
 */
void send_wake_end_signal() {
    send_command(wake_pipe, ac::protocol::wake::Command::shutdown);
}
