import std;

import auto_core.core.pipes;
import auto_core.core.config;
import auto_core.taskbar;
import command_registry;
import journal_commands;
import journal_component;
import component_protocol;

import <Windows.h>;

namespace {

int write_manifest(
    const command_registry::Registry& registry,
    const std::filesystem::path& destination
) {
    std::filesystem::path temporary = destination;
    temporary += ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        return 1;
    }

    for (const std::string& value : registry.autocomplete_values()) {
        output << value << '\n';
    }
    output.close();
    if (!output) {
        return 1;
    }

    return MoveFileExW(
        temporary.c_str(),
        destination.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
    ) ? 0 : 1;
}

} // namespace

int main(int argument_count, char* arguments[]) {
    ac::config::initialize_core_settings();
    auto registry = create_journal_command_registry();

    if (argument_count == 3 &&
        std::string_view {arguments[1]} ==
            "--generate-keymap-command-registry") {
        return write_manifest(registry, arguments[2]);
    }

    journal_component().connect_to_logger();
    journal_component().log_and_log("journal_ac.exe started");

    if (!ac::taskbar::connect()) {
        journal_component().log_and_print(
            "Unable to receive the native taskbar snapshot; interactive "
            "prompts will use direct console activation."
        );
    }
    struct TaskbarConnectionGuard {
        ~TaskbarConnectionGuard() { ac::taskbar::disconnect(); }
    } taskbar_connection_guard;

    auto connection = ac::pipes::connect_to_pipe_server(
        ac::protocol::component::pipe_name("journal")
    );
    if (!connection) {
        journal_component().log_and_print(
            "Failed to connect to journal pipe. Error: {}",
            connection.error().system_error
        );
        return 1;
    }

    ac::pipes::Pipe pipe = std::move(*connection);
    ac::pipes::CommandDispatcher dispatcher;
    bool protocol_failed = false;
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::invoke
        ),
        [&pipe, &registry, &dispatcher, &protocol_failed] {
            const auto expression = ac::pipes::read_string(pipe);
            if (!expression) {
                journal_component().log_and_print(
                    "Failed to read journal command. Error: {}",
                    expression.error().system_error
                );
                protocol_failed = true;
                dispatcher.request_stop();
                return;
            }

            auto action = registry.resolve(*expression);
            if (!action) {
                journal_component().log_and_print(
                    "Unknown journal command: {}",
                    *expression
                );
                return;
            }
            action();
        }
    );
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::shutdown
        ),
        [&dispatcher] {
            journal_component().log_and_log("shutdown signal received");
            dispatcher.request_stop();
        }
    );

    if (const auto ready = ac::pipes::send_string(
            pipe, ac::protocol::component::make_hello(
                registry.autocomplete_values()
            )
        ); !ready) {
        journal_component().log_and_print(
            "Failed to signal journal readiness. Error: {}",
            ready.error().system_error
        );
        return 1;
    }

    if (const auto result = dispatcher.process(pipe); !result) {
        journal_component().log_and_print(
            "Journal pipe failed. Error: {}",
            result.error().system_error
        );
        return 1;
    }

    if (protocol_failed) {
        return 1;
    }

    journal_component().log_and_log("program terminated");
    return 0;
}
