/**
 * \file defaults.ixx
 * \brief Portable defaults for Main `_config.exe` files.
 *
 * Tracked samples under `defaults/` must match these strings.
 */
export module auto_core.main.defaults;

import std;

export namespace ac::main::defaults {

    constexpr bool warn_without_winkey_mapping = true;

    constexpr std::string_view auto_core_ini =
        "# The presence of this file indicates that Auto Core has been initialized.\n"
        "\n"
        "[auto_core]\n"
        "warn_without_winkey_mapping = true\n";

    constexpr std::string_view components_new_components = "on";
    constexpr bool components_sort = true;
    constexpr bool components_remove_missing = false;

    constexpr std::string_view components_ini =
        "[settings]\n"
        "new_components = on\n"
        "sort_components = on\n"
        "remove_missing_components = off\n";

    constexpr std::string_view components_list =
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

    constexpr std::string_view crash_default_response = "no";

    constexpr std::string_view crash_recovery_ini =
        "[dialog]\n"
        "default_response = no\n";

    constexpr std::string_view shutdown_prompt = "popup";
    constexpr unsigned shutdown_timeout_ms = 2000;

    constexpr std::string_view shutdown_ini =
        "[shutdown]\n"
        "delayed_shutdown_prompt = popup\n"
        "shutdown_timeout_ms = 2000\n";

    constexpr std::string_view logger_directory = "logs";
    constexpr std::uint64_t logger_merge_interval_seconds = 60;
    constexpr bool logger_merge_logs_on_shutdown = true;
    constexpr bool logger_write_logs_to_console = false;

    constexpr std::string_view logger_ini =
        "[logger]\n"
        "directory = logs\n"
        "merge_interval_seconds = 60\n"
        "merge_logs_on_shutdown = on\n"
        "write_logs_to_console = off\n";

    constexpr bool keymap_silence_nonset_warning = false;

    constexpr std::string_view keymap_ini =
        "[keymap]\n"
        "silence_nonset_warning = false\n";

    [[nodiscard]]
    inline std::string ini_for_auto_core(const bool warn) {
        if (warn == warn_without_winkey_mapping) {
            return std::string {auto_core_ini};
        }
        return std::string {
            "# The presence of this file indicates that Auto Core has been initialized.\n"
            "\n"
            "[auto_core]\n"
            "warn_without_winkey_mapping = false\n"
        };
    }

    [[nodiscard]]
    inline std::string ini_for_components() {
        return std::string {components_ini};
    }

    [[nodiscard]]
    inline std::string ini_for_crash_recovery(const std::string_view response) {
        if (response == crash_default_response) {
            return std::string {crash_recovery_ini};
        }
        return std::string {
            "[dialog]\n"
            "default_response = "
        } + std::string {response} + "\n";
    }

    [[nodiscard]]
    inline std::string ini_for_shutdown(
        const std::string_view prompt,
        const unsigned timeout_ms
    ) {
        if (prompt == shutdown_prompt && timeout_ms == shutdown_timeout_ms) {
            return std::string {shutdown_ini};
        }
        return std::string {
            "[shutdown]\n"
            "delayed_shutdown_prompt = "
        } + std::string {prompt} +
            "\n"
            "shutdown_timeout_ms = " +
            std::to_string(timeout_ms) +
            "\n";
    }

    [[nodiscard]]
    inline std::string ini_for_logger(
        const std::string_view stored_directory,
        const std::uint64_t merge_interval_seconds,
        const bool merge_logs_on_shutdown,
        const bool write_logs_to_console
    ) {
        const std::string directory = stored_directory.empty()
            ? std::string {logger_directory}
            : std::string {stored_directory};
        if (directory == logger_directory &&
            merge_interval_seconds == logger_merge_interval_seconds &&
            merge_logs_on_shutdown == logger_merge_logs_on_shutdown &&
            write_logs_to_console == logger_write_logs_to_console) {
            return std::string {logger_ini};
        }
        return std::string {
            "[logger]\n"
            "directory = "
        } + directory +
            "\n"
            "merge_interval_seconds = " +
            std::to_string(merge_interval_seconds) +
            "\n"
            "merge_logs_on_shutdown = " +
            (merge_logs_on_shutdown ? "on" : "off") +
            "\n"
            "write_logs_to_console = " +
            (write_logs_to_console ? "on" : "off") +
            "\n";
    }

    [[nodiscard]]
    inline std::string ini_for_keymap(const bool silence) {
        if (silence == keymap_silence_nonset_warning) {
            return std::string {keymap_ini};
        }
        return std::string {
            "[keymap]\n"
            "silence_nonset_warning = true\n"
        };
    }

} // namespace ac::main::defaults
