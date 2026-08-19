/**
\file main.cxx
\brief Logs system wake events, extending Windows' native logging to offer historical wake event tracking.

By polling system information, this module provides detailed logging of wake events,
aiding in the analysis of system behavior and Auto Core's interaction with the host machine.
*/
import std;
import auto_core.pipes;
import wake_logging;
import wake_protocol;

/**
 * \brief Ends the wake component.
 */
void end_wake() {
    wake_component.logg("wake.exe is shutting down");
}

/**
 * \brief Sets up the command map for handling pipe commands.
 *
 * This function maps integer command IDs to corresponding functions that handle
 * specific commands for the iTunes component.
 */
void set_commands(ac::pipes::CommandDispatcher& dispatcher) {
    dispatcher.set_command(ac::protocol::wake::to_wire(ac::protocol::wake::Command::shutdown), [&dispatcher]() {
        end_wake();
        dispatcher.request_stop();
    });
    dispatcher.set_command(ac::protocol::wake::to_wire(ac::protocol::wake::Command::log_last_wake), log_last_wake);
    dispatcher.set_command(ac::protocol::wake::to_wire(ac::protocol::wake::Command::update_component), update_wake_component);
}

int main() {
    log_init();
    log_last_wake();
    ac::pipes::CommandDispatcher dispatcher;
    set_commands(dispatcher);

    ac::pipes::Pipe wake_pipe;
    auto connection = ac::pipes::connect_to_pipe_server(
        std::wstring { ac::protocol::wake::pipe_name }
    );

    if (connection) {
        wake_pipe = std::move(*connection);
        if (const auto result = dispatcher.process(wake_pipe);
            !result) {
            wake_component.logg_and_print(
                "Wake pipe failed. Error: {}",
                result.error().system_error
            );
        }
    }
    else {
        wake_component.logg_and_print(
            "Failed to connect to wake pipe. Error: {}",
            connection.error().system_error
        );
    }

    wake_component.logg_and_logg("wake.exe has ended");

    return 0;
}
