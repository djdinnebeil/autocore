import std;

import auto_core.pipes;
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

    for (int index = 1; index + 1 < argument_count; ++index) {
        if (std::string_view {arguments[index]} != "--taskbar-position") {
            continue;
        }
        int position = -1;
        const std::string_view value {arguments[index + 1]};
        const auto parsed = std::from_chars(
            value.data(), value.data() + value.size(), position
        );
        if (parsed.ec == std::errc {} &&
            parsed.ptr == value.data() + value.size() &&
            position >= 0 && position <= 9) {
            set_journal_taskbar_position(position);
        }
        break;
    }

    journal_component().connect_to_logger();
    journal_component().logg_and_logg("journal.exe started");

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

    journal_component().logg_and_logg("journal.exe ended");
    return 0;
}
