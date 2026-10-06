/**
 * \file program_ready.ixx
 * \brief Main startup-status output.
 */
export module auto_core.main.program_ready;

/**
 * Prints ready status and the weekday on a detached thread so the message
 * loop can start immediately.
 *
 * The task section is an intentional Main ready-banner feature. It reads
 * Writer's `task_list.txt` directly. It is not component dispatch, and
 * disabling Writer does not remove the banner.
 */
export void announce_program_ready();
/** Prints the current weekday name. */
export void print_today_is_day();
