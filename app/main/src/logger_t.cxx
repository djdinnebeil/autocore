module logger_t;
import std;
import config;
import clock;
import logger;
import logger_c;
import logger_x;
import <Windows.h>;

namespace fs = std::filesystem;
namespace this_thread = std::this_thread;
namespace chrono = std::chrono;

/**
 * \brief Logging thread function.
 *
 * This function runs in a separate thread and logs activities at regular intervals.
 * It updates log files and manages log rotation based on the current date.
 */
void log_thread() {
    ac::Logger logger_logger("logger");
    int log_thread_sleep_time = 60;
    int log_thread_after_60_minutes = 60;
    int log_minutes_count = 0;
    std::string current_datestamp = ac::clock::get_datestamp();
    logger_logger.logg_and_logg("logger thread started");
    while (true) {
        int seconds_to_sleep = log_thread_sleep_time - ac::clock::get_current_seconds();
        send_logg_wake_signal();
        log_minutes_count++;
        if (log_minutes_count == log_thread_after_60_minutes) {
            logger_logger.logg_and_logg("{} minutes have passed - log thread sleeping for {} seconds at {}", log_thread_after_60_minutes, seconds_to_sleep, ac::clock::get_timestamp_with_seconds());
            log_minutes_count = 0;
        }
        else {
            logger_logger.logg("log thread sleeping for {} seconds at {}", seconds_to_sleep, ac::clock::get_timestamp_with_seconds());
        }
        this_thread::sleep_for(chrono::seconds(seconds_to_sleep));
        current_datestamp = ac::clock::get_datestamp();
        if (current_datestamp != logger_datestamp) {
            ac::logger::logg("---");
            ac::logger::update_main_log_file();
            logger_logger.update_log_file();
            update_log_components();
            ac::logger::logg("---");
            ac::logger::logg(session_start);
        }
    }
}

/**
 * \brief Initializes the logging system.
 *
 * This function sets up the logging environment by setting the session start time, creating necessary
 * directories, updating the main log file, and starting the logging thread.
 */
void log_init() {
    session_start = "Session started " + ac::clock::get_datetime_stamp_for_logger();
    fs::create_directories(log_directory);
    ac::logger::update_main_log_file();
    if (ac::config.send_logg_to_cout) {
        ac::logger::logg("***send logg to output enabled***");
    }
    ac::logger::loggnl(ac::config.configuration_log);
    std::thread t(log_thread);
    t.detach();
}

/**
 * \brief Starts the logging session.
 *
 * This function initializes the session start time, creates necessary directories,
 * updates the main log file, and logs the session start message.
 */
void log_start() {
    session_start = "Session started " + ac::clock::get_datetime_stamp_for_logger();
    fs::create_directories(log_directory);
    ac::logger::update_main_log_file();
    ac::logger::logg(session_start);
}
