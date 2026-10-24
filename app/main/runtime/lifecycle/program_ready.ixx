/**
 * \file program_ready.ixx
 * \brief Main startup-status output.
 */
export module auto_core.main.program_ready;

/**
 * Prints the configured ready status before post-ready background work starts.
 *
 * `config/startup.ini` independently controls the weekday and task sections.
 * The task section remains an intentional Main ready-banner feature. When it
 * is enabled, Main reads Writer's `task_list.txt` directly. It is not component
 * dispatch, and disabling Writer does not remove the banner.
 */
export void announce_program_ready();
