/**
\file server_x.ixx
\brief Provides logging support and initialization for the Spotify component.

This module defines a custom logger for the Spotify component and provides functions
to update the logger and initialize logging when the component starts.
 */
export module server_logging;
import std;
import config;
import clock;
import logger;
import logger_c;
import print;

/**
 * \brief Custom logger for the server component.
 *
 * This logger is used to log messages specific to the Spotify component.
 */
export ac::Logger server_logger("server");

/**
 * \brief Updates the server logger.
 *
 * This function updates the main log file and the server logger file.
 */
export void update_server_logger() {
	ac::logger::update_main_log_file();
	server_logger.update_log_file();
}

/**
 * \brief Initializes logging for the server component.
 *
 * This function updates the main log file and logs the start of the server component.
 */
export void log_init() {
	ac::logger::update_main_log_file();
	server_logger.logg_and_logg("server.exe started");
}
