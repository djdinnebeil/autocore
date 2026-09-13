/**
 * \file program_ready.ixx
 * \brief Main startup-status output.
 */
export module auto_core.main.program_ready;

/**
 * Prints ready status, weekday, and `writer/task_list.txt` on a detached
 * thread so the message loop can start immediately.
 */
export void announce_program_ready();
/** Prints the current weekday name. */
export void print_today_is_day();
