/**
 * \file keymap.ixx
 * \brief Shared keymap types and active bindings.
 */
export module keymap;

import std;

export struct function_state {
    std::function<void()> primary;
    std::function<void()> secondary;
};

export std::unordered_map<int, function_state> ac_numkey_event;
