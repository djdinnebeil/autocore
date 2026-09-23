/**
 * \file config_defaults.hpp
 * \brief Portable default bytes for live config files.
 *
 * Tracked files under `defaults/` must match these strings. Host INI files
 * are written only by the matching `_config.exe`. Component INI defaults
 * live in each child's `shared/defaults.ixx`. Runtime never reads repo
 * `defaults/`.
 */
#pragma once

#include <string_view>

namespace ac::config::detail {

    inline constexpr std::string_view auto_core_ini =
        "# This file is not parsed.\n"
        "# auto_core.exe checks only for this file's existence.\n"
        "\n"
        "[auto_core]\n"
        "initialized = true\n";

    inline constexpr std::string_view main_ini =
        "[main]\n"
        "warn_without_winkey_mapping = true\n";

    inline constexpr std::string_view components_ini =
        "[settings]\n"
        "new_components = on\n"
        "sort_components = on\n"
        "remove_missing_components = off\n";

    inline constexpr std::string_view components_list =
        "# Leave blank for on; use explicit on/off to override\n"
        "[components]\n"
        "dash\n"
        "itunes\n"
        "journal\n"
        "logger\n"
        "server\n"
        "slash\n"
        "spotify\n"
        "taskbar\n"
        "wake\n"
        "writer\n";

    inline constexpr std::string_view crash_recovery_ini =
        "[dialog]\n"
        "default_response = no\n";

    inline constexpr std::string_view shutdown_ini =
        "[shutdown]\n"
        "delayed_shutdown_prompt = popup\n"
        "shutdown_timeout_ms = 2000\n";

    inline constexpr std::string_view journal_choices_ini =
        "; Short names for keymap.map. The right-hand side is a journal factory\n"
        "; expression. Unused names are not bound. journal_config.exe writes\n"
        "; journal_choices.ini once if it is missing. An existing\n"
        "; journal_choices.ini is never overwritten.\n"
        "; Auto Core never reads repo defaults/.\n"
        "\n"
        "print_Tabby_choice = make_print_choice(\"Tabby\", 0, 1)\n"
        "print_one_is_selected = print_and_insert_into_journal(\"1 is selected.\")\n";

    inline constexpr std::string_view keymap_ini =
        "[keymap]\n"
        "trace_enabled = false\n"
        "silence_nonset_warning = false\n";

} // namespace ac::config::detail
