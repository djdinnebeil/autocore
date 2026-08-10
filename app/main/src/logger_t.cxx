module logger_t;

import std;

import config;
import clock;
import ac_component;
import logger;
import logger_x;
import session;
import journey;

import pipes;
import <Windows.h>;

namespace fs = std::filesystem;
namespace this_thread = std::this_thread;
namespace chrono = std::chrono;

namespace {
    ac::session::Start session_start;
    std::string logger_date;
}

void log_init() {
    start_logger_component();

    session_start = ac::session::make_start();

    fs::create_directories(
        fs::path {
            ac::config::logger_directory()
        }
    );

    Sleep(100);
    ac::logger::connect_to_logger(auto_core);

    auto_core.logg_and_logg(
        "Main session started {}",
        session_start.datetime
    );

    if (ac::config::send_logg_to_cout()) {
        auto_core.logg_and_logg(
            "***send logg to output enabled***"
        );
    }

    auto_core.loggnl_and_loggnl(
        std::string {
            ac::config::configuration_log()
        }
    );

}

void start_logger_component() {
    std::wstring sp_path = LR"(.\logger.exe)";
    ac::main::create_process(sp_path);
}
