/**
 * \file wake_logging.ixx
 * \brief Wake-event logging for `wake_ac.exe`.
 *
 * `log_last_wake` runs `powercfg /lastwake` and compares the output against
 * the previous capture under the configured log directory's
 * `components/wake/` folder.
 */
export module wake_logging;

import std;
import auto_core.core.logging.config;
import auto_core.core.clock;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.paths;

namespace fs = std::filesystem;

export ac::Component wake_component("wake");

export void update_wake_component() {
	wake_component.update_log_file();
}

export void log_init() {
    wake_component.connect_to_logger();
	wake_component.log_and_log("wake_ac.exe started");
    const auto ini_path = ac::paths::config_directory() / "wake.ini";
    if (!ac::ini::read(ini_path)) {
        std::error_code exists_error;
        const bool present =
            std::filesystem::exists(ini_path, exists_error);
        wake_component.report_ini_unavailable(present && !exists_error);
    }
}

/**
 * Captures `powercfg /lastwake` into `wake_latest.log`. On change, appends
 * to `wake_master.log` and updates `wake_previous.log`.
 */
export void log_last_wake() {
    wake_component.log(
        "Checking last wake log at {}",
        ac::clock::get_timestamp_with_seconds()
    );

    const fs::path wake_directory =
        ac::logging::config::components_directory() /
        "wake";

    fs::create_directories(wake_directory);

    const fs::path previous_last_wake_file =
        wake_directory / "wake_previous.log";

    const fs::path current_last_wake_file =
        wake_directory / "wake_latest.log";

    const fs::path last_wake_log_file =
        wake_directory / "wake_master.log";

    {
        std::ofstream current_last_wake_clear(
            current_last_wake_file
        );

        if (!current_last_wake_clear.is_open()) {
            wake_component.log(
                "Unable to clear '{}'.",
                current_last_wake_file.string()
            );

            return;
        }
    }

    const std::string retrieve_last_wake_command =
        std::format(
            "powercfg /lastwake >> \"{}\"",
            current_last_wake_file.string()
        );

    system(retrieve_last_wake_command.c_str());

    std::ifstream previous_last_wake_stream(
        previous_last_wake_file
    );

    std::string line;
    std::ostringstream previous_last_wake_oss;

    while (std::getline(previous_last_wake_stream, line)) {
        previous_last_wake_oss << line << '\n';
    }

    std::ifstream current_last_wake_stream(
        current_last_wake_file
    );

    std::ostringstream current_last_wake_oss;

    while (std::getline(current_last_wake_stream, line)) {
        current_last_wake_oss << line << '\n';
    }

    const std::string previous_last_wake_str =
        previous_last_wake_oss.str();

    const std::string current_last_wake_str =
        current_last_wake_oss.str();

    if (current_last_wake_str != previous_last_wake_str) {
        const std::string current_last_wake_output =
            ac::clock::get_datetime() +
            '\n' +
            current_last_wake_str;

        {
            std::ofstream last_wake_log_stream(
                last_wake_log_file,
                std::ios::app
            );

            if (last_wake_log_stream.is_open()) {
                last_wake_log_stream
                    << current_last_wake_output;
            }
        }

        {
            std::ofstream previous_last_wake_update(
                previous_last_wake_file
            );

            if (previous_last_wake_update.is_open()) {
                previous_last_wake_update
                    << current_last_wake_str;
            }
        }

        wake_component.lognl_and_lognl(
            "wake state change detected at {}",
            current_last_wake_output
        );
    }

    wake_component.flush();
}
