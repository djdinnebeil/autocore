module logger_init;

import std;

import ac_main;
import auto_core.clock;
import auto_core.logging.config;
import journey;
import auto_core.paths;

namespace fs = std::filesystem;

namespace {

    void start_logger_component() {
        auto_core.logg("Starting logger.exe");

        const std::filesystem::path logger_path =
            ac::paths::executable_directory() / "logger.exe";

        ac::main::create_process(logger_path);
    }

}

void initialize_logger_component() {
    auto_core.logg(
        "Main session started {}",
        ac::clock::format_datetime(auto_core.session_start())
    );

    fs::create_directories(
        ac::logging::config::directory()
    );

    if (ac::logging::config::enabled()) {
        start_logger_component();
        auto_core.connect_to_logger();
    }
    else {
        auto_core.logg("Central logging is disabled");
    }

    if (ac::logging::config::write_to_console()) {
        auto_core.logg_and_logg(
            "***send logg to output enabled***"
        );
    }

    auto_core.loggnl_and_loggnl(
        std::string {ac::logging::config::configuration_report()}
    );
}
