/**
 * \file config_defaults.hpp
 * \brief Portable default bytes for live INI files.
 *
 * Tracked files under `defaults/` must match these strings. Auto Core writes
 * a missing live file from these bytes and never reads repo `defaults/`.
 */
#pragma once

#include <string_view>

namespace ac::config::detail {

    inline constexpr std::string_view auto_core_ini =
        "[auto_core]\n"
        "warn_without_winkey_mapping = true\n";

    inline constexpr std::string_view logger_ini =
        "[logger]\n"
        "directory = logs\n"
        "enabled = true\n"
        "write_to_console = false\n";

    inline constexpr std::string_view server_ini =
        "[server]\n"
        "port = 8585\n"
        "document_root = server\n";

    inline constexpr std::string_view crash_recovery_ini =
        "[dialog]\n"
        "default_response = no\n";

    inline constexpr std::string_view itunes_ini =
        "[itunes]\n"
        "auto_start = false\n"
        "tab_end = 3\n";

    inline constexpr std::string_view journal_ini =
        "[journal]\n"
        "directory = journal\n"
        "series =\n"
        "remote_sync = disable\n"
        "\n"
        "[timestamp]\n"
        "day_rollover_hour = 0\n";

    inline constexpr std::string_view journal_choices_ini =
        "; Short names for bindings.ini. The right-hand side is a journal factory\n"
        "; expression. Unused names are not bound. Auto Core writes journal_choices.ini\n"
        "; once if it is missing. An existing journal_choices.ini is never overwritten.\n"
        "; Auto Core never reads repo defaults/.\n"
        "\n"
        "print_Tabby_choice = make_print_choice(\"Tabby\", 0, 1)\n"
        "print_one_is_selected = print_and_insert_into_journal(\"1 is selected.\")\n";

    inline constexpr std::string_view taskbar_ini =
        "[taskbar]\n"
        "directory = taskbar\n"
        "mode = live\n";

    inline constexpr std::string_view keymap_ini =
        "[keymap]\n"
        "trace_enabled = false\n"
        "silence_nonset_warning = false\n";

    inline constexpr std::string_view spotify_ini =
        "[spotify]\n"
        "directory = spotify\n";

    inline constexpr std::string_view writer_ini =
        "[writer]\n"
        "directory = writer\n"
        "notes_directory = notes\n";

} // namespace ac::config::detail
