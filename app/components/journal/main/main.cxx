import std;

import auto_core.core.pipes;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.taskbar;
import command_registry;
import journal_cloud_client;
import journal_commands;
import journal_component;
import journal_db_client;
import journal_remote_sync;
import component_protocol;

import <Windows.h>;
import auto_core.core.shell;

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
    ac::shell::set_process_app_user_model_id();
    auto registry = create_journal_command_registry();

    if (argument_count == 3 &&
        std::string_view {arguments[1]} ==
            "--generate-keymap-command-registry") {
        return write_manifest(registry, arguments[2]);
    }

    journal_component().log_main("journal_ac.exe started");

    if (const auto database = journal::db::start_service(); !database) {
        journal_component().log("{}", database.error());
    }
    struct DatabaseServiceGuard {
        ~DatabaseServiceGuard() { journal::db::shutdown_service(); }
    } database_service_guard;

    {
        const auto ini_path = ac::paths::config_directory() / "journal.ini";
        const auto document = ac::ini::read(ini_path);
        if (!document) {
            std::error_code exists_error;
            const bool present =
                std::filesystem::exists(ini_path, exists_error);
            journal_component().report_ini_unavailable(
                present && !exists_error
            );
        }
        else if (const auto remote_sync = document->find("journal", "remote_sync");
                 remote_sync && journal::remote_sync::enabled(*remote_sync)) {
            if (const auto cloud = journal::cloud::start_service(); !cloud) {
                journal_component().log("{}", cloud.error());
            }
        }
    }
    struct CloudServiceGuard {
        ~CloudServiceGuard() { journal::cloud::shutdown_service(); }
    } cloud_service_guard;

    if (!ac::taskbar::connect()) {
        journal_component().log_print(
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
        journal_component().log_print(
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
                journal_component().log_print(
                    "Failed to read journal command. Error: {}",
                    expression.error().system_error
                );
                protocol_failed = true;
                dispatcher.request_stop();
                return;
            }

            auto action = registry.resolve(*expression);
            if (!action) {
                journal_component().log_print(
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
            journal_component().log_main("shutdown signal received");
            dispatcher.request_stop();
        }
    );

    if (const auto ready = ac::pipes::send_string(
            pipe, ac::protocol::component::make_hello(
                registry.autocomplete_values()
            )
        ); !ready) {
        journal_component().log_print(
            "Failed to signal journal readiness. Error: {}",
            ready.error().system_error
        );
        return 1;
    }

    if (const auto result = dispatcher.process(pipe); !result) {
        journal_component().log_print(
            "Journal pipe failed. Error: {}",
            result.error().system_error
        );
        return 1;
    }

    if (protocol_failed) {
        return 1;
    }

    journal_component().log_main("program terminated");
    return 0;
}
