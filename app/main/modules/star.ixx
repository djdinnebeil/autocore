/**
\file star.ixx
\brief Assists in the file creation and management of journaling documents within Auto Core.

This module provides functionality for managing journaling documents, including generating
episode titles, saving files, and interacting with a SQLite database.
*/
export module star;

export import command_registry;

export namespace star::runtime_commands {
    void register_with(command_registry::Registry& registry);
}

export {
    void print_episode_title();
    void save_file_and_create_new_file();
}
