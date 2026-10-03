/**
 * \file wake_logging.ixx
 * \brief Wake-event capture for `wake_ac.exe`.
 *
 * `log_last_wake` runs `powercfg /lastwake` and compares the output against
 * `previous.event` under `[wake] directory`. Those history files are
 * operational state. Ordinary component logs still follow `logging.ini`.
 */
module;

#include "wake_capture.hpp"
#include "../shared/wake_history_detail.hpp"

export module wake_logging;

import std;
import auto_core.core.logging.config;
import auto_core.core.clock;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.paths;

namespace fs = std::filesystem;

export ac::Component wake_component(
    "wake",
    ac::logging::config::LoggingScope {"wake"}
);

export void log_init() {
	wake_component.log_main("wake_ac.exe started");
    const auto ini_path = ac::paths::config_directory() / "wake.ini";
    if (!ac::ini::read(ini_path)) {
        std::error_code exists_error;
        const bool present =
            std::filesystem::exists(ini_path, exists_error);
        wake_component.report_ini_unavailable(present && !exists_error);
    }
}

/**
 * Captures `powercfg /lastwake` and updates `current.event`. On a real
 * change, appends to `wake_events.log` and updates `previous.event`.
 *
 * The command finishes before `current.event` is modified. A failed or
 * empty capture leaves an existing `current.event` in place and creates
 * any missing history file empty.
 *
 * \return `0` when the capture was applied or the empty-file fallback
 *         succeeded. `1` when the directory or a history write fails.
 */
export int log_last_wake() {
    wake_component.log(
        "Checking last wake log at {}",
        ac::clock::get_timestamp_with_seconds()
    );

    const fs::path wake_directory = ac::paths::wake_directory();
    std::error_code create_error;
    fs::create_directories(wake_directory, create_error);
    if (create_error) {
        wake_component.log(
            "Failed to create Wake directory: {}",
            create_error.message()
        );
        wake_component.flush();
        return 1;
    }

    const auto captured = wake::capture_powercfg();
    if (!captured) {
        wake_component.log("Unable to capture the last wake event.");
        const bool created =
            wake::history::create_missing_history_files(wake_directory);
        if (!created) {
            wake_component.log("Unable to create Wake history files.");
        }
        wake_component.flush();
        return created ? 0 : 1;
    }

    const auto stamp = ac::clock::get_datetime();
    const auto result = wake::history::apply_capture(
        wake_directory,
        *captured,
        stamp
    );
    if (result == wake::history::ApplyResult::failed) {
        wake_component.log("Unable to update Wake history.");
        wake_component.flush();
        return 1;
    }
    if (result == wake::history::ApplyResult::recorded) {
        wake_component.lognl_main(
            "wake state change detected at {}\n{}",
            stamp,
            wake::history::normalize_body(*captured)
        );
    }

    wake_component.flush();
    return 0;
}
