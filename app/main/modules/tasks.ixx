/**
 * \file link.ixx
 * \brief Generates custom user-defined strings for use within the Auto Core application.
 *
 * This module provides functions to generate and print messages for journaling inspiration
 * and task lists. It interacts with the clipboard and logs the generated messages.
 */
export module tasks;

export import command_registry;
import std;

export namespace tasks::runtime_commands {
    void register_with(command_registry::Registry& registry);
}

export {
    void launch_task_list();
    void print_task_list();
    std::string get_task_list();
}
