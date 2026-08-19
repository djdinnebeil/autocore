 /**
 \file main.cxx
 \brief This file contains the implementation of the server component using CivetServer.

 The server component initializes and runs a local server based on the configuration
 specified in the server.ini file. This enables local file access through a web browser.

 \warning Ensure that the configuration file server.ini is properly formatted
 and located in the config directory.
 */
import std;

import auto_core.encoding;
import auto_core.ini;
import auto_core.paths;

import server_logging;

#pragma warning(disable:4251)
#pragma warning(disable:4275)
import <CivetServer.h>;

namespace this_thread = std::this_thread;
namespace chrono = std::chrono;

CivetServer* server;

/**
 * \brief Runs the server.
 * Initializes the server with the configuration options and starts the server loop.
 */
void run_server() {
    int configured_port = 8585;
    if (const auto document = ac::ini::read(
        ac::paths::config_directory() / "server.ini"
    )) {
        if (const auto value = document->find("server", "port")) {
            int parsed_port = 0;
            const auto result = std::from_chars(
                value->data(), value->data() + value->size(), parsed_port
            );
            if (result.ec == std::errc {} &&
                result.ptr == value->data() + value->size() &&
                parsed_port >= 1 && parsed_port <= 65535) {
                configured_port = parsed_port;
            }
        }
    }

    const std::string port_number = std::to_string(configured_port);

    const std::filesystem::path document_root_path =
        ac::paths::executable_directory() / "server";

    const std::string document_root =
        ac::encoding::to_utf8(document_root_path.native());

    const char* options[] = {
        "document_root", document_root.c_str(),
        "listening_ports", port_number.c_str(),
        nullptr
    };

    CivetServer server(options);

    server_component.logg_and_logg(
        "Server started on port {}",
        port_number
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
