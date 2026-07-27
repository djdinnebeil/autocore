 /**
 \file main.cxx
 \brief This file contains the implementation of the server component using CivetServer.

 The server component initializes and runs a local server based on the configuration
 specified in the server.ini file. This enables local file access through a web browser.

 \warning Ensure that the configuration file server.ini is properly formatted
 and located in the config directory.
 */
import std;
import config;
import clock;
import logger;
import clipboard;
import print;
import utils;
import encoding;
import keyboard;
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
    std::string port_number_str = std::to_string(ac::config.port_number);
    const char* options[] = {
        "document_root", R"(.\server\)",
        "listening_ports", port_number_str.c_str(),
        nullptr
    };
    std::string current_datestamp = ac::clock::get_datestamp();
    try {
        server = new CivetServer(options);
        server_logger.logg_and_logg("server started on port {}", options[3]);
        while (true) {
            this_thread::sleep_for(chrono::seconds(60));
            std::string datestamp = ac::clock::get_datestamp();
            if (datestamp != current_datestamp) {
                server_logger.update_log_file();
                current_datestamp = datestamp;
            }
        }
    }
    catch (std::exception& e) {
        ac::print("Exception caught in server: {}", e.what());
        exit(1);
    }
}

int main() {
    log_init();
    run_server();
    return 0;
}