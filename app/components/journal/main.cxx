import std;

import auto_core.core.pipes;
import auto_core.core.config;
import auto_core.taskbar;
import command_registry;
import journal_commands;
import journal_component;
import journal_protocol;

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
    auto registry = create_journal_command_registry();

    if (argument_count == 3 &&
        std::string_view {arguments[1]} ==
            "--generate-keymap-command-registry") {
        return write_manifest(registry, arguments[2]);
    }

    ac::config::initialize_core_settings();

    journal_component().connect_to_logger();
    journal_component().logg_and_logg("journal_ac.exe started");

    if (!ac::taskbar::connect()) {
        journal_component().logg_and_print(
            "Unable to receive the native taskbar snapshot; interactive "
            "prompts will use direct console activation."
        );
    }
    struct TaskbarConnectionGuard {
        ~TaskbarConnectionGuard() { ac::taskbar::disconnect(); }
    } taskbar_connection_guard;

    auto connection = ac::pipes::connect_to_pipe_server(
        std::wstring {ac::protocol::journal::pipe_name}
    );
    if (!connection) {
        journal_component().logg_and_print(
            "Failed to connect to journal pipe. Error: {}",
            connection.error().system_error
        );
        return 1;
    }

    ac::pipes::Pipe pipe = std::move(*connection);
    ac::pipes::CommandDispatcher dispatcher;
    bool protocol_failed = false;
    dispatcher.set_command(
        ac::protocol::journal::to_wire(
            ac::protocol::journal::Request::invoke
        ),
        [&pipe, &registry, &dispatcher, &protocol_failed] {
            const auto expression = ac::pipes::read_string(pipe);
            if (!expression) {
                journal_component().logg_and_print(
                    "Failed to read journal command. Error: {}",
                    expression.error().system_error
                );
                protocol_failed = true;
                dispatcher.request_stop();
                return;
            }

            auto action = registry.resolve(*expression);
            if (!action) {
                journal_component().logg_and_print(
                    "Unknown journal command: {}",
                    *expression
                );
                return;
            }
            action();
        }
    );
    dispatcher.set_command(
        ac::protocol::journal::to_wire(
            ac::protocol::journal::Request::shutdown
        ),
        [&dispatcher] { dispatcher.request_stop(); }
    );

    if (const auto ready = ac::pipes::send_string(
            pipe, ac::protocol::journal::ready_message
        ); !ready) {
        journal_component().logg_and_print(
            "Failed to signal journal readiness. Error: {}",
            ready.error().system_error
        );
        return 1;
    }

    if (const auto result = dispatcher.process(pipe); !result) {
        journal_component().logg_and_print(
            "Journal pipe failed. Error: {}",
            result.error().system_error
        );
        return 1;
    }

    if (protocol_failed) {
        return 1;
    }

    journal_component().logg_and_logg("journal_ac.exe ended");
    return 0;
}
