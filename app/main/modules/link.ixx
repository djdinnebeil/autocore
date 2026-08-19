/**
 * \file link.ixx
 * \brief Generates custom user-defined strings for use within the Auto Core application.
 *
 * This module provides functions to generate and print messages for journaling inspiration
 * and task lists. It interacts with the clipboard and logs the generated messages.
 */
export module link;

export import command_registry;

export namespace link::runtime_commands {
    void register_with(command_registry::Registry& registry);
}

export {
    void print_gpt_message();
}
