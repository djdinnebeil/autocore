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
        "[auto_core]\n"
        "warn_without_winkey_mapping = on\n"
        "logging = on\n";

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

    inline constexpr std::string_view keymap_ini =
        "[keymap]\n"
        "silence_nonset_warning = off\n";

} // namespace ac::config::detail
