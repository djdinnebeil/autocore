/**
\file spotify_component.ixx
\brief Provides logging support and initialization for the Spotify component.

This module defines a custom logger for the Spotify component and provides functions
to update the logger and initialize logging when the component starts.
 */
export module spotify_component;

import auto_core.core.component;

import auto_core.core.pipes;
import <Windows.h>;

/**
 * \brief Custom logger for the Spotify component.
 *
 * This logger is used to log messages specific to the Spotify component.
 */
export ac::Component spotify_component("spotify");

/**
 * \brief Updates the Spotify logger.
 *
 * This function updates the main log file and the Spotify logger file.
 */
export void update_spotify_component() {
	spotify_component.update_log_file();
}

/**
 * \brief Initializes logging for the Spotify component.
 *
 * This function updates the main log file and logs the start of the Spotify component.
 */
export void log_init() {
	spotify_component.connect_to_logger();
	spotify_component.log_and_log("spotify_ac.exe started");
}
