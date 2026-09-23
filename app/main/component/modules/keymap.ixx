/**
 * \file keymap.ixx
 * \brief Shared keymap types and active bindings.
 */
export module auto_core.main.keymap;

import std;
import auto_core.main.key_codes;

export struct key_binding {
    /** Used when `primary` is true. */
    std::function<void()> primary;
    /** Used after `activate_function_key()` until deactivated. */
    std::function<void()> secondary;
};

/** Filled at keymap init; consumed by `process_key_event`. */
export std::unordered_map<key_codes::Code, key_binding> active_keymap;
