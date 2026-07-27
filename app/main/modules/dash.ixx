/**
 * \file dash.ixx
 * \brief This module is used to initialize runtime configuration, if enabled.
 *
 * This module reads configuration settings from the keymap.ini file and sets
 * action mappings based on the configuration. It is only used if runtime configuration
 * is enabled.
 */
export module dash;
import ac_modules;
import keymap;
import dash_x;
import <Windows.h>;

export {
    void set_keymap_from_file();
}
