/**
 * \file server_logging.ixx
 * \brief Component logging for `server_ac.exe`.
 */
export module server_logging;

import std;
import auto_core.core.component;

import auto_core.core.pipes;
import <Windows.h>;

export ac::Component server_component("server");

export void update_server_component() {
	server_component.update_log_file();
}

export void log_init() {
	server_component.log_main("server_ac.exe started");
}
