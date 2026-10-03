/**
 * \file server_logging.ixx
 * \brief Component logging for `server_ac.exe`.
 */
export module server_logging;

import std;
import auto_core.core.component;
import auto_core.core.logging.config;

export ac::Component server_component(
    "server",
    ac::logging::config::LoggingScope {"server"}
);

export void log_init() {
	server_component.log_main("server_ac.exe started");
}
