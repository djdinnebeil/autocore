/**
 * \file dash.ixx
 * \brief This module is used to initialize the keymap from file, if enabled.
 *
 * This module reads key mappings from keymap.ini and applies the configured
 * actions. It is used only when file-based keymap loading is enabled.
 */
export module dash;

export {
    void set_keymap_from_file();
}
