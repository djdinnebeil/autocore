/**
 * \file main_log.ixx
 * \brief Writes decoded central-log events to the logger's local files.
 */
export module main_log;

import auto_core.core.logging.protocol;

export void write_to_main_log(
    const ac::logging::Event& event
);
