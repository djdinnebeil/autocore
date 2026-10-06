/**
 * \file logging_init.ixx
 * \brief Opens and closes Main's local session log.
 */
export module auto_core.main.logging;

/**
 * Creates the log directory and writes Main's session-start record.
 *
 * Shared logging policy is resolved by `main` before this runs.
 */
export void initialize_logging();
/**
 * Writes Main's shutdown record once.
 */
export void shutdown_logging();
