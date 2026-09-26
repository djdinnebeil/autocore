/**
 * \file logger_init.ixx
 * \brief Opens and closes Main's local session log.
 */
export module auto_core.main.logger;

/**
 * Creates the log directory and writes Main's session-start record.
 */
export void initialize_logger_component();
/**
 * Writes Main's shutdown record once.
 */
export void shutdown_logger_component();
