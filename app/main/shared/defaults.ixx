/**
 * \file defaults.ixx
 * \brief Portable defaults for Main `_config.exe` files.
 *
 * Tracked samples under `defaults/` must match these strings.
 */
export module auto_core.main.defaults;

import std;

export namespace ac::main::defaults {

    constexpr std::string_view auto_core_ini =
        "# This file is not parsed.\n"
        "# auto_core.exe checks only for this file's existence.\n"
        "\n"
        "[auto_core]\n"
        "initialized = true\n";

    constexpr bool warn_without_winkey_mapping = true;

    constexpr std::string_view main_ini =
        "[main]\n"
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

    constexpr bool keymap_trace_enabled = false;
    constexpr bool keymap_silence_nonset_warning = false;

    constexpr std::string_view keymap_ini =
        "[keymap]\n"
        "trace_enabled = false\n"
        "silence_nonset_warning = false\n";

    [[nodiscard]]
    inline std::string ini_for_main(const bool warn) {
        if (warn == warn_without_winkey_mapping) {
            return std::string {main_ini};
        }
        return std::string {
            "[main]\n"
            "warn_without_winkey_mapping = "
        } + (warn ? "true" : "false") + "\n";
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
    inline std::string ini_for_keymap(const bool trace, const bool silence) {
        if (trace == keymap_trace_enabled &&
            silence == keymap_silence_nonset_warning) {
            return std::string {keymap_ini};
        }
        const char* trace_text = trace ? "true" : "false";
        const char* silence_text = silence ? "true" : "false";
        return std::string {
            "[keymap]\n"
            "trace_enabled = "
        } + trace_text +
            "\n"
            "silence_nonset_warning = " +
            silence_text +
            "\n";
    }

} // namespace ac::main::defaults
