/**
 * \file logger_init.ixx
 * \brief Starts and stops `logger_ac.exe` from the Main process.
 */
export module auto_core.main.logger;

/**
 * Creates the log directory, launches `logger_ac.exe` when `logger` is
 * enabled in `components.list`, then calls `connect_to_logger()`.
 */
export void initialize_logger_component();
/**
 * Requests a graceful logger stop and waits for the configured shutdown
 * deadline. Force-terminates the process if that deadline expires.
 */
export void shutdown_logger_component();
