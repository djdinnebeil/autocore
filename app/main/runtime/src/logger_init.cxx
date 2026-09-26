module auto_core.main.logger;

import std;

import auto_core.main.application;
import auto_core.core.clock;
import auto_core.core.logging.config;

namespace fs = std::filesystem;

namespace {

    std::atomic_bool logger_shutdown_started {false};

}

void initialize_logger_component() {
    auto_core.log(
        "Main session started {}",
        ac::clock::format_datetime(auto_core.session_start())
    );

    fs::create_directories(
        ac::logging::config::directory()
    );

    if (ac::logging::config::ini_missing()) {
        auto_core.log_print(
            "config/logger.ini is missing. Run logger_config.exe to "
            "generate it. Using built-in defaults; the file will not be "
            "created."
        );
    }

    auto_core.lognl_main(
        std::string {ac::logging::config::configuration_report()}
    );
}

void shutdown_logger_component() {
    if (logger_shutdown_started.exchange(true)) {
        return;
    }

    auto_core.log_main("Auto Core is shutting down");
}
