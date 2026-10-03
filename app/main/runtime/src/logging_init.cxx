module auto_core.main.logging;

import std;

import auto_core.main.application;
import auto_core.core.clock;
import auto_core.core.logging.config;

namespace fs = std::filesystem;

namespace {

    std::atomic_bool logging_shutdown_started {false};

}

void initialize_logging() {
    auto_core.log(
        "Main session started {}",
        ac::clock::format_datetime(auto_core.session_start())
    );

    if (!ac::logging::config::disable_all() &&
        ac::logging::config::component_logging_default() &&
        ac::logging::config::write_logs_to_files()) {
        fs::create_directories(
            ac::logging::config::directory()
        );
    }

    auto_core.lognl_main(
        std::string {ac::logging::config::configuration_report()}
    );
}

void shutdown_logging() {
    if (logging_shutdown_started.exchange(true)) {
        return;
    }

    auto_core.log_main("Auto Core is shutting down");
}
