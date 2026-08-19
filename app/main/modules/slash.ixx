/**
 * \file slash.ixx
 * \brief Manages the emptying of the Recycle Bin.
 *
 * This module provides functionality to retrieve and delete the contents of the Recycle Bin.
 * It utilizes an external executable to perform the deletion.
 */
export module slash;

export import command_registry;

export namespace slash::runtime_commands {
    void register_with(command_registry::Registry& registry);
}

export void retrieve_and_delete_recycle_bin();
