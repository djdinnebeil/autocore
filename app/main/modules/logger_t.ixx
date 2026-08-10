/**
 * \file logger_t.ixx
 * \brief This module creates a logger thread, updates component logs, and initializes logging.
 *
 * This module defines functions that create and manage a logging thread. The thread
 * logs activities at regular intervals, updates log files, and manages log rotation.
 * It also initializes the logging system.
 */
export module logger_t;

export {
    void log_init();
    void update_log_components();
    void start_logger_component();
}
