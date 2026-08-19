/**
\file sp_x.ixx
\brief Provides logging support and initialization for the Spotify component.

This module defines a custom logger for the Spotify component and provides functions
to update the logger and initialize logging when the component starts.
 */
export module sp_x;

import auto_core.component;

import auto_core.pipes;
import <Windows.h>;

/**
 * \brief Custom logger for the Spotify component.
 *
 * This logger is used to log messages specific to the Spotify component.
 */
export ac::Component sp_component("sp");

/**
 * \brief Updates the Spotify logger.
 *
 * This function updates the main log file and the Spotify logger file.
 */
export void update_sp_component() {
	sp_component.update_log_file();
}

/**
 * \brief Initializes logging for the Spotify component.
 *
 * This function updates the main log file and logs the start of the Spotify component.
 */
export void log_init() {
	sp_component.connect_to_logger();
	sp_component.logg_and_logg("sp_ac.exe started");
}
