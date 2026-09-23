/**
 * \file main_log.ixx
 * \brief Writes decoded central-log events to the logger's local files.
 */
export module main_log;

import std;
import auto_core.core.logging.protocol;

export void write_to_main_log(
    const ac::logging::Event& event
);

/**
 * Writes the terminal centralized-log entry and rejects all later entries.
 * The supplied component name is rendered by the normal log-prefix path.
 */
export void shutdown_main_log(const ac::logging::Event& request);
