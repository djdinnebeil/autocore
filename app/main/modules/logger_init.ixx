/**
 * \file logger_init.ixx
 * \brief Starts and stops `logger_ac.exe` from the Main process.
 */
export module auto_core.main.logger;

/**
 * Creates the log directory, launches `logger_ac.exe` when `logger.ini`
 * `enabled` is true, then calls `connect_to_logger()`.
 */
export void initialize_logger_component();
/**
 * Requests a graceful logger stop, waits six seconds, then terminates
 * the process if it is still running.
 */
export void shutdown_logger_component();
