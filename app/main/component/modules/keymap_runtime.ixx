/**
 * \file keymap_runtime.ixx
 * \brief Initializes the keymap from `keymap.map`.
 *
 * Each action is resolved independently: a valid name still binds when the
 * other side is empty or unknown. Unknown expressions log at load and again
 * on press of that side. `primary | secondary` and omitted keys stay out of the map (the
 * physical key passes through); after a successful load they print "hasn't
 * been set" unless `silence_nonset_warning` is `true`. `keymap_editor.exe`
 * writes `keymap.map` when missing. File-load failure installs a two-key
 * emergency map in memory and does not write the file.
 */
export module auto_core.main.keymap.runtime;

import std;

export {
    /** Loads `keymap.map` into `active_keymap`. */
    void initialize_keymap();
    /** Parses `keymap.map` into `active_keymap`. Failure installs the emergency map. */
    void set_keymap_from_file();
    /** Names written to `keymap_commands.txt`. No in-repo caller. */
    std::vector<std::string> get_runtime_command_names();
    /** Autocomplete expressions written to `keymap_commands.txt`. No in-repo caller. */
    std::vector<std::string> get_runtime_command_autocomplete_values();
}
