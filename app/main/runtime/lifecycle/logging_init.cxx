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
    auto_core.log_debug(
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

    if (const auto report = ac::logging::config::configuration_report();
        !report.empty()) {
        auto_core.lognl_info(std::string {report});
    }
}

void shutdown_logging() {
    if (logging_shutdown_started.exchange(true)) {
        return;
    }

    auto_core.log_info("Auto Core is shutting down");
}
