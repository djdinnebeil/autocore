/**
\file server_x.ixx
\brief Provides logging support and initialization for the Spotify component.

This module defines a custom logger for the Spotify component and provides functions
to update the logger and initialize logging when the component starts.
 */
export module server_logging;

import std;
import auto_core.component;

import auto_core.pipes;
import <Windows.h>;

/**
 * \brief Custom logger for the server component.
 *
 * This logger is used to log messages specific to the Spotify component.
 */
export ac::Component server_component("server");

/**
 * \brief Updates the server logger.
 *
 * This function updates the main log file and the server logger file.
 */
export void update_server_component() {
	server_component.update_log_file();
}

/**
 * \brief Initializes logging for the server component.
 *
 * This function updates the main log file and logs the start of the server component.
 */
export void log_init() {
	server_component.connect_to_logger();
	server_component.logg_and_logg("server.exe started");
}
