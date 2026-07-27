/**
\file wake_logging.ixx
\brief Provides logging support and initialization for the Spotify component.

This module defines a custom logger for the Spotify component and provides functions
to update the logger and initialize logging when the component starts.
 */
export module wake_logging;
import std;
import config;
import clock;
import logger;
import logger_c;
import print;

namespace fs = std::filesystem;

/**
 * \brief Custom logger for the Spotify component.
 *
 * This logger is used to log messages specific to the Spotify component.
 */
export ac::Logger wake_logger("wake");

/**
 * \brief Updates the Spotify logger.
 *
 * This function updates the main log file and the Spotify logger file.
 */
export void update_wake_logger() {
    ac::logger::update_main_log_file();
	wake_logger.update_log_file();
}

/**
 * \brief Initializes logging for the Spotify component.
 *
 * This function updates the main log file and logs the start of the Spotify component.
 */
export void log_init() {
    ac::logger::update_main_log_file();
	wake_logger.logg_and_logg("wake.exe started");
}

/**
 * \brief Logs the last wake event.
 *
 * This function retrieves the last wake event information from the system and logs it.
 * If a change in wake state is detected, it updates the log files and logs the new wake event.
 */
export void log_last_wake() {
    wake_logger.logg("Checking last wake log at {}", ac::clock::get_timestamp_with_seconds());
    std::string wake_directory = ac::config.logger_directory + R"(wake\)";
    fs::create_directories(wake_directory);
    static std::string previous_last_wake_file = wake_directory + "previous_wake.log";
    static std::string current_last_wake_file = wake_directory + "current_wake.log";
    static std::string last_wake_log_file = wake_directory + "wake_master_log.log";
    std::ofstream current_last_wake_clear(current_last_wake_file);
    if (current_last_wake_clear.is_open()) {
        current_last_wake_clear << "";
        current_last_wake_clear.flush();
        current_last_wake_clear.close();
    }
    static std::string retrieve_last_wake_command = std::format("powercfg /lastwake >> \"{}\"", current_last_wake_file);
    system(retrieve_last_wake_command.c_str());
    std::ifstream previous_last_wake_stream(previous_last_wake_file);
    std::string line;
    std::ostringstream previous_last_wake_oss;
    while (std::getline(previous_last_wake_stream, line)) {
        previous_last_wake_oss << line << '\n';
    }
    previous_last_wake_stream.close();
    std::ifstream current_last_wake_stream(current_last_wake_file);
    std::ostringstream current_last_wake_oss;
    while (std::getline(current_last_wake_stream, line)) {
        current_last_wake_oss << line << '\n';
    }
    current_last_wake_stream.close();
    std::string current_last_wake_str = current_last_wake_oss.str();
    std::string current_last_wake_output;
    if (current_last_wake_str != previous_last_wake_oss.str()) {
        current_last_wake_output = ac::clock::get_datetime_stamp_with_seconds() + '\n' + current_last_wake_str;
        std::ofstream last_wake_log_stream(last_wake_log_file, std::ios::app);
        if (last_wake_log_stream.is_open()) {
            last_wake_log_stream << current_last_wake_output;
            last_wake_log_stream.flush();
            last_wake_log_stream.close();
        }
        std::ofstream previous_last_wake_update(previous_last_wake_file);
        if (previous_last_wake_update.is_open()) {
            previous_last_wake_update << current_last_wake_str;
            previous_last_wake_update.flush();
            previous_last_wake_update.close();
        }
        wake_logger.loggnl_and_loggnl("wake state change detected at {}", current_last_wake_output);
    }
    wake_logger.log_stream.flush();
    main_log_stream.flush();
}
