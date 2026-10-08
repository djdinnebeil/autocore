/**
 * \file main.cxx
 * \brief Local loopback file server for `server_ac.exe`.
 *
 * Resolves `[server] directory` from `config/server.ini`, then requires
 * `port.id` and `document_root.id`. A relative document root resolves
 * under that directory. A missing document-root directory fails startup.
 * This process does not write those files and does not launch
 * `server_config.exe`, `server_editor.exe`, or `server_builder.exe`.
 */
#include "../shared/server_data_detail.hpp"

import std;

import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.core.pipes;
import component_protocol;

import server_logging;

#pragma warning(disable:4251)
#pragma warning(disable:4275)
import <CivetServer.h>;
import auto_core.core.shell;

namespace {

    std::string_view trim(std::string_view value) {
        const auto first = value.find_first_not_of(" \t");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t");
        return value.substr(first, last - first + 1);
    }

    struct Launch {
        int port {0};
        std::filesystem::path document_root;
    };

    std::filesystem::path resolve_data_directory() {
        const auto ini_path = ac::paths::config_directory() / "server.ini";
        std::optional<std::string> stored;
        if (const auto document = ac::ini::read(ini_path)) {
            if (const auto value = document->find("server", "directory")) {
                const auto text = trim(*value);
                if (!text.empty()) {
                    stored = std::string {text};
                }
            }
        }
        else {
            std::error_code exists_error;
            const bool present = std::filesystem::exists(ini_path, exists_error);
            server_component.report_ini_unavailable(present && !exists_error);
        }

        std::optional<std::string_view> view;
        if (stored) {
            view = *stored;
        }
        return server::data::resolve_directory(
            view,
            ac::paths::installation_root()
        );
    }

    enum class ScalarFile {
        missing,
        unreadable,
        ok
    };

    struct ScalarText {
        ScalarFile status {ScalarFile::missing};
        std::string text;
    };

    ScalarText read_scalar(const std::filesystem::path& path) {
        std::error_code error;
        const bool present = std::filesystem::exists(path, error);
        if (error) {
            return {ScalarFile::unreadable, {}};
        }
        if (!present) {
            return {ScalarFile::missing, {}};
        }
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            return {ScalarFile::unreadable, {}};
        }
        std::ostringstream contents;
        contents << input.rdbuf();
        return {ScalarFile::ok, contents.str()};
    }

    std::optional<Launch> prepare_launch() {
        const auto data_directory = resolve_data_directory();

        const auto port_text = read_scalar(data_directory / "port.id");
        const auto port = port_text.status == ScalarFile::ok
            ? server::data::parse_port(port_text.text)
            : std::nullopt;
        if (!port) {
            server_component.log_print(
                "Server port.id is missing or invalid. Run server_editor.exe to configure Server."
            );
            return std::nullopt;
        }

        const auto root_text = read_scalar(data_directory / "document_root.id");
        const auto stored = root_text.status == ScalarFile::ok
            ? server::data::parse_document_root(root_text.text)
            : std::nullopt;
        if (!stored) {
            server_component.log_print(
                "Server document_root.id is missing or invalid. Run server_editor.exe to configure Server."
            );
            return std::nullopt;
        }

        const bool absolute = server::data::document_root_is_absolute(*stored);
        const auto document_root = server::data::resolve_document_root(
            *stored,
            data_directory
        );
        std::error_code exists_error;
        const bool exists = std::filesystem::exists(document_root, exists_error);
        if (exists_error) {
            server_component.log_print(
                "Failed to inspect {}: {}",
                document_root.string(),
                exists_error.message()
            );
            return std::nullopt;
        }

        switch (server::data::document_root_startup(exists)) {
        case server::data::DocumentRootStartup::serve:
            break;
        case server::data::DocumentRootStartup::fail:
            if (absolute) {
                server_component.log_print(
                    "Server document root is missing: {}",
                    document_root.string()
                );
            }
            else {
                server_component.log_print(
                    "Server document root is missing: {}. Run server_builder.exe --seed to create starter files.",
                    document_root.string()
                );
            }
            return std::nullopt;
        }

        return Launch {*port, document_root};
    }

    void serve(
        const Launch& launch,
        std::atomic_bool& stop_requested
    ) {
        const std::string port_number = std::to_string(launch.port);
        const std::string listening_ports = "127.0.0.1:" + port_number;
        const std::string document_root =
            ac::encoding::to_utf8(launch.document_root.native());

        const char* options[] = {
            "document_root", document_root.c_str(),
            "listening_ports", listening_ports.c_str(),
            nullptr
        };

        try {
            CivetServer server(options);

            server_component.log_main(
                "Server started at http://127.0.0.1:{}/ serving {}",
                port_number,
                launch.document_root
            );

            while (true) {
                if (stop_requested.load()) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
                server_component.update_log_file();
            }
        }
        catch (const std::exception& exception) {
            server_component.log_print(
                "Failed to bind http://127.0.0.1:{}/: {}",
                port_number,
                exception.what()
            );
        }
        catch (...) {
            server_component.log_print(
                "Failed to bind http://127.0.0.1:{}/",
                port_number
            );
        }
    }

    bool run_control_pipe(
        const Launch& launch,
        std::atomic_bool& stop_requested
    ) {
        auto connection = ac::pipes::connect_to_pipe_server(
            ac::protocol::component::pipe_name("server")
        );
        if (!connection) {
            server_component.log_print(
                "Failed to connect to the server control pipe. Error: {}",
                connection.error().system_error
            );
            return false;
        }

        ac::pipes::Pipe pipe = std::move(*connection);
        ac::pipes::CommandDispatcher dispatcher;
        dispatcher.set_command(
            ac::protocol::component::to_wire(
                ac::protocol::component::Request::shutdown
            ),
            [&dispatcher, &stop_requested] {
                server_component.log_main(
                    "shutdown signal received - force termination allowed"
                );
                stop_requested.store(true);
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
                server_component.log_print(
                    "Unknown server command: {}",
                    *expression
                );
            }
        );

        if (const auto hello = ac::pipes::send_string(
                pipe,
                ac::protocol::component::make_hello(
                    {},
                    ac::protocol::component::TerminationPolicy::force_allowed
                )
            ); !hello) {
            server_component.log_print(
                "Failed to send server hello. Error: {}",
                hello.error().system_error
            );
            stop_requested.store(true);
            return false;
        }

        std::jthread http {[&launch, &stop_requested] {
            serve(launch, stop_requested);
        }};
        (void)dispatcher.process(pipe);
        stop_requested.store(true);
        return true;
    }

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    if (argc == 2 &&
        std::string_view {argv[1]} == "--export-keymap-commands") {
        return 0;
    }
    log_init();
    std::atomic_bool stop_requested {false};

    const auto launch = prepare_launch();
    if (!launch) {
        return 1;
    }
    if (!run_control_pipe(*launch, stop_requested)) {
        return 1;
    }

    server_component.log_main("program terminated");
    return 0;
}
