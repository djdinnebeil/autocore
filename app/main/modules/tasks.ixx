/**
 * \file link.ixx
 * \brief Generates custom user-defined strings for use within the Auto Core application.
 *
 * This module provides functions to generate and print messages for journaling inspiration
 * and task lists. It interacts with the clipboard and logs the generated messages.
 */
export module tasks;

import std;

export {
    void launch_task_list();
    void print_task_list();
    std::string get_task_list();
}
