/**
 * \file main.cxx
 * \brief Local loopback file server for `server_ac.exe`.
 *
 * Reads `config/server.ini` and serves `[server] document_root` on
 * `127.0.0.1` at `[server] port`. Relative roots resolve against the
 * installation root. Missing or empty settings keep `server` and `8585`.
 */
import std;

import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.core.pipes;
import component_protocol;
import server_defaults;

import server_logging;

#pragma warning(disable:4251)
#pragma warning(disable:4275)
import <CivetServer.h>;

namespace {

    int read_port(const ac::ini::Document& document) {
        int configured_port = server::defaults::port;
        if (const auto value = document.find("server", "port")) {
            int parsed_port = 0;
            const auto result = std::from_chars(
                value->data(),
                value->data() + value->size(),
                parsed_port
            );
            if (result.ec == std::errc {} &&
                result.ptr == value->data() + value->size() &&
                parsed_port >= 1 &&
                parsed_port <= 65535) {
                configured_port = parsed_port;
            }
        }
        return configured_port;
    }

    std::filesystem::path read_document_root(
        const ac::ini::Document& document
    ) {
        std::filesystem::path root =
            ac::paths::installation_root() / server::defaults::document_root;
        const auto value = document.find("server", "document_root");
        if (!value || value->empty()) {
            return root;
        }

        std::filesystem::path configured {
            ac::encoding::to_utf16(*value)
        };
        if (configured.is_relative()) {
            configured = ac::paths::installation_root() / configured;
        }
        return configured.lexically_normal();
    }

}

/**
 * \brief Runs the server.
 * Initializes the server with the configuration options and starts the server loop.
 */
void run_server(std::atomic_bool& stop_requested) {
    int configured_port = server::defaults::port;
    std::filesystem::path document_root_path =
        ac::paths::installation_root() / server::defaults::document_root;

    const auto ini_path = ac::paths::config_directory() / "server.ini";
    if (const auto document = ac::ini::read(ini_path)) {
        configured_port = read_port(*document);
        document_root_path = read_document_root(*document);
    }
    else {
        std::error_code exists_error;
        const bool present = std::filesystem::exists(ini_path, exists_error);
        server_component.report_ini_unavailable(present && !exists_error);
    }

    const std::string port_number = std::to_string(configured_port);
    const std::string listening_ports = "127.0.0.1:" + port_number;
    const std::string document_root =
        ac::encoding::to_utf8(document_root_path.native());

    std::error_code exists_error;
    if (!std::filesystem::exists(document_root_path, exists_error)) {
        server_component.log_main(
            "Document root is missing: {}",
            document_root_path
        );
    }

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
            document_root_path
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

bool run_control_pipe(std::atomic_bool& stop_requested) {
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

    std::jthread http {[&stop_requested] {
        run_server(stop_requested);
    }};
    (void)dispatcher.process(pipe);
    stop_requested.store(true);
    return true;
}

int main() {
    log_init();
    std::atomic_bool stop_requested {false};

    if (!run_control_pipe(stop_requested)) {
        return 1;
    }

    server_component.log_main("program terminated");
    return 0;
}
