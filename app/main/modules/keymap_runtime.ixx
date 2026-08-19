/**
 * \file keymap_runtime.ixx
 * \brief This module is used to initialize the keymap from file, if enabled.
 *
 * This module reads key mappings from keymap.ini and applies the configured
 * actions. It is used only when file-based keymap loading is enabled.
 */
export module keymap_runtime;

import std;

export {
    void initialize_keymap();
    void set_keymap_from_file();
    std::vector<std::string> get_runtime_command_names();
    std::vector<std::string> get_runtime_command_autocomplete_values();
}
