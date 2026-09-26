/**
\file main.cxx
\brief Logs system wake events, extending Windows' native logging to offer historical wake event tracking.

By polling system information, this module provides detailed logging of wake events,
aiding in the analysis of system behavior and Auto Core's interaction with the host machine.
*/
import std;
import auto_core.core.pipes;
import wake_logging;
import component_protocol;

int main() {
    log_init();
    log_last_wake();

    auto connection = ac::pipes::connect_to_pipe_server(
        ac::protocol::component::pipe_name("wake")
    );
    if (!connection) {
        wake_component.log_print(
            "Failed to connect to wake pipe. Error: {}",
            connection.error().system_error
        );
        wake_component.log_main("wake_ac.exe has ended");
        return 1;
    }

    ac::pipes::Pipe pipe = std::move(*connection);
    ac::pipes::CommandDispatcher dispatcher;
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::shutdown
        ),
        [&dispatcher] {
            wake_component.log_main("shutdown signal received");
            dispatcher.request_stop();
        }
    );
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::invoke
        ),
        [&pipe, &dispatcher] {
            const auto expression = ac::pipes::read_string(pipe);
            if (!expression) {
                dispatcher.request_stop();
                return;
            }
            wake_component.log_print(
                "Unknown wake command: {}",
                *expression
            );
        }
    );

    if (const auto hello = ac::pipes::send_string(
            pipe, ac::protocol::component::make_hello({})
        ); !hello) {
        wake_component.log_print(
            "Failed to send wake hello. Error: {}",
            hello.error().system_error
        );
        return 1;
    }

    if (const auto result = dispatcher.process(pipe); !result) {
        wake_component.log_print(
            "Wake pipe failed. Error: {}",
            result.error().system_error
        );
    }

    wake_component.log_main("program terminated");
    return 0;
}
