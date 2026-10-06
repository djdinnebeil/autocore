import std;
import auto_core.core.component;
import auto_core.core.pipes;
import auto_core.taskbar;
import command_registry;
import writer_commands;
import writer_component;
import component_protocol;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;

import <Windows.h>;
import auto_core.core.shell;

namespace {

void report_invalid_notes_subdirectory() {
    const auto document = ac::ini::read(
        ac::paths::config_directory() / "writer.ini"
    );
    if (!document) {
        return;
    }
    const auto value = document->find("writer", "notes_subdirectory");
    if (!value || value->empty()) {
        return;
    }

    bool relative = false;
    try {
        const std::filesystem::path configured {
            ac::encoding::to_utf16(std::string {*value})
        };
        relative = !configured.empty() &&
            !configured.has_root_name() &&
            !configured.has_root_directory();
    }
    catch (...) {
        relative = false;
    }
    if (relative) {
        return;
    }
    writer_component().log_print(
        "config/writer.ini has an invalid notes_subdirectory. "
        "Using the notes folder under the Writer directory. "
        "The file will not be rewritten."
    );
}

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
    ac::shell::set_process_app_user_model_id();
    auto registry = create_writer_command_registry();

    if (argument_count == 3 &&
        std::string_view {arguments[1]} ==
            "--generate-keymap-command-registry") {
        return write_manifest(registry, arguments[2]);
    }

    writer_component().log_main("writer_ac.exe started");

    {
        const auto ini_path = ac::paths::config_directory() / "writer.ini";
        if (!ac::ini::read(ini_path)) {
            std::error_code exists_error;
            const bool present =
                std::filesystem::exists(ini_path, exists_error);
            writer_component().report_ini_unavailable(
                present && !exists_error
            );
        }
        else {
            report_invalid_notes_subdirectory();
        }
    }

    if (!ac::taskbar::connect()) {
        writer_component().log_print(
            "Unable to receive the native taskbar snapshot; notes will "
            "open without taskbar pre-activation."
        );
    }
    struct TaskbarConnectionGuard {
        ~TaskbarConnectionGuard() { ac::taskbar::disconnect(); }
    } taskbar_connection_guard;

    auto connection = ac::pipes::connect_to_pipe_server(
        ac::protocol::component::pipe_name("writer")
    );
    if (!connection) {
        writer_component().log_print(
            "Failed to connect to writer pipe. Error: {}",
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
                writer_component().log_print(
                    "Failed to read writer command. Error: {}",
                    expression.error().system_error
                );
                protocol_failed = true;
                dispatcher.request_stop();
                return;
            }
            auto action = registry.resolve(*expression);
            if (!action) {
                writer_component().log_print(
                    "Unknown writer command: {}", *expression
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
            writer_component().log_main("shutdown signal received");
            dispatcher.request_stop();
        }
    );

    if (const auto ready = ac::pipes::send_string(
            pipe, ac::protocol::component::make_hello(
                registry.autocomplete_values()
            )
        ); !ready) {
        writer_component().log_print(
            "Failed to signal writer readiness. Error: {}",
            ready.error().system_error
        );
        return 1;
    }

    if (const auto result = dispatcher.process(pipe); !result) {
        writer_component().log_print(
            "Writer pipe failed. Error: {}",
            result.error().system_error
        );
        return 1;
    }
    if (protocol_failed) {
        return 1;
    }

    writer_component().log_main("program terminated");
    return 0;
}
