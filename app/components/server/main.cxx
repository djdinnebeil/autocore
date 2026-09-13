/**
 * \file main.cxx
 * \brief Local loopback file server for `server_ac.exe`.
 *
 * Reads `config/server.ini` and serves `[server] document_root` on
 * `127.0.0.1` at `[server] port`. Relative roots resolve against the
 * executable directory. Missing or empty settings keep `server` and `8585`.
 */
import std;

import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;

import server_logging;

#pragma warning(disable:4251)
#pragma warning(disable:4275)
import <CivetServer.h>;

namespace {

    int read_port(const ac::ini::Document& document) {
        int configured_port = 8585;
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
            ac::paths::executable_directory() / "server";
        const auto value = document.find("server", "document_root");
        if (!value || value->empty()) {
            return root;
        }

        std::filesystem::path configured {
            ac::encoding::to_utf16(*value)
        };
        if (configured.is_relative()) {
            configured = ac::paths::executable_directory() / configured;
        }
        return configured.lexically_normal();
    }

}

/**
 * \brief Runs the server.
 * Initializes the server with the configuration options and starts the server loop.
 */
void run_server() {
    int configured_port = 8585;
    std::filesystem::path document_root_path =
        ac::paths::executable_directory() / "server";

    if (const auto document = ac::ini::read(
        ac::paths::config_directory() / "server.ini"
    )) {
        configured_port = read_port(*document);
        document_root_path = read_document_root(*document);
    }

    const std::string port_number = std::to_string(configured_port);
    const std::string listening_ports = "127.0.0.1:" + port_number;
    const std::string document_root =
        ac::encoding::to_utf8(document_root_path.native());

    std::error_code exists_error;
    if (!std::filesystem::exists(document_root_path, exists_error)) {
        server_component.logg_and_logg(
            "Document root is missing: {}",
            document_root_path
        );
    }

    const char* options[] = {
        "document_root", document_root.c_str(),
        "listening_ports", listening_ports.c_str(),
        nullptr
    };

    CivetServer server(options);

    server_component.logg_and_logg(
        "Server started at http://127.0.0.1:{}/ serving {}",
        port_number,
        document_root_path
    );

    while (true) {
        std::this_thread::sleep_for(
            std::chrono::minutes(1)
        );

        server_component.update_log_file();
    }
}

int main() {
    log_init();
    run_server();
    return 0;
}
