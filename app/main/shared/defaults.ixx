/**
 * \file defaults.ixx
 * \brief Portable defaults for Main `_config.exe` files.
 *
 * Tracked samples under `defaults/` must match these strings.
 */
export module auto_core.main.defaults;

import std;

export namespace ac::main::defaults {

    constexpr bool auto_core_logging = true;

    constexpr bool startup_show_day_of_week = true;
    constexpr bool startup_show_task_list = true;

    constexpr std::string_view auto_core_ini =
        "[auto_core]\n"
        "logging = on\n";

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
    constexpr bool crash_diagnostics = true;

    constexpr std::string_view crash_recovery_ini =
        "[crash_recovery]\n"
        "default_response = no\n"
        "crash_diagnostics = on\n";

    constexpr std::string_view shutdown_prompt = "popup";
    constexpr unsigned shutdown_timeout_ms = 2000;

    constexpr std::string_view shutdown_ini =
        "[shutdown]\n"
        "delayed_shutdown_prompt = popup\n"
        "shutdown_timeout_ms = 2000\n";

    constexpr bool logging_disable_all = false;
    inline const std::filesystem::path logging_directory =
        std::filesystem::path {"data"} / "logs";
    inline const std::string logging_directory_text =
        logging_directory.string();
    constexpr std::int64_t logging_max_directory_mb = 100;
    constexpr std::int64_t logging_retention_days = 30;
    constexpr std::string_view logging_level = "debug";
    constexpr bool logging_log_console_output = true;
    constexpr bool logging_write_logs_to_files = true;
    constexpr bool logging_write_logs_to_console = false;
    constexpr bool logging_write_alerts_to_console = true;
    constexpr bool logging_component_logging_default = true;
    constexpr bool logging_experimental = false;

    constexpr std::string_view logging_ini =
        "[logging]\n"
        "disable_all = off\n"
        "directory = data\\logs\n"
        "max_directory_mb = 100\n"
        "retention_days = 30\n"
        "level = debug\n"
        "log_console_output = on\n"
        "write_logs_to_files = on\n"
        "write_logs_to_console = off\n"
        "write_alerts_to_console = on\n"
        "component_logging_default = on\n"
        "experimental = off\n";

    constexpr std::uint64_t logger_merge_interval_seconds = 60;
    constexpr bool logger_merge_logs_on_shutdown = true;

    constexpr std::string_view logger_ini =
        "[logger]\n"
        "merge_interval_seconds = 60\n"
        "merge_logs_on_shutdown = on\n"
        "logging = on\n";

    constexpr bool warnings_missing_winkey_mapping = true;
    constexpr bool warnings_unmapped_key = true;

    constexpr std::string_view warnings_ini =
        "[warnings]\n"
        "missing_winkey_mapping = on\n"
        "unmapped_key = on\n";

    [[nodiscard]]
    inline std::string ini_for_auto_core(
        const bool logging = auto_core_logging
    ) {
        if (logging == auto_core_logging) {
            return std::string {auto_core_ini};
        }
        return std::string {
            "[auto_core]\n"
            "logging = "
        } + std::string {logging ? "on" : "off"} +
            "\n";
    }

    [[nodiscard]]
    inline std::string ini_for_startup(
        const bool show_day_of_week = startup_show_day_of_week,
        const bool show_task_list = startup_show_task_list
    ) {
        return std::string {
            "show_day_of_week = "
        } + (show_day_of_week ? "on" : "off") +
            "\nshow_task_list = " +
            (show_task_list ? "on" : "off") +
            "\n";
    }

    [[nodiscard]]
    inline std::string ini_for_components() {
        return std::string {components_ini};
    }

    [[nodiscard]]
    inline std::string ini_for_crash_recovery(
        const std::string_view response,
        const bool diagnostics = crash_diagnostics
    ) {
        if (response == crash_default_response &&
            diagnostics == crash_diagnostics) {
            return std::string {crash_recovery_ini};
        }
        return std::string {
            "[crash_recovery]\n"
            "default_response = "
        } + std::string {response} +
            "\ncrash_diagnostics = " +
            (diagnostics ? "on" : "off") +
            "\n";
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
    inline std::string ini_for_logging(
        const bool disable_all,
        const std::string_view stored_directory,
        const std::int64_t max_directory_mb,
        const std::int64_t retention_days,
        const std::string_view level,
        const bool log_console_output,
        const bool write_logs_to_files,
        const bool write_logs_to_console,
        const bool write_alerts_to_console,
        const bool component_logging_default,
        const bool experimental
    ) {
        const std::string directory = stored_directory.empty()
            ? logging_directory_text
            : std::string {stored_directory};
        const std::string stored_level =
            level == "debug" || level == "info" || level == "alert"
                ? std::string {level}
                : std::string {logging_level};
        const std::int64_t stored_cap = max_directory_mb < 0
            ? logging_max_directory_mb
            : max_directory_mb;
        const std::int64_t stored_retention = retention_days < 0
            ? logging_retention_days
            : retention_days;
        if (disable_all == logging_disable_all &&
            directory == logging_directory_text &&
            stored_cap == logging_max_directory_mb &&
            stored_retention == logging_retention_days &&
            stored_level == logging_level &&
            log_console_output == logging_log_console_output &&
            write_logs_to_files == logging_write_logs_to_files &&
            write_logs_to_console == logging_write_logs_to_console &&
            write_alerts_to_console == logging_write_alerts_to_console &&
            component_logging_default == logging_component_logging_default &&
            experimental == logging_experimental) {
            return std::string {logging_ini};
        }
        return std::string {
            "[logging]\n"
            "disable_all = "
        } + (disable_all ? "on" : "off") +
            "\n"
            "directory = " + directory +
            "\n"
            "max_directory_mb = " + std::to_string(stored_cap) +
            "\n"
            "retention_days = " + std::to_string(stored_retention) +
            "\n"
            "level = " + stored_level +
            "\n"
            "log_console_output = " +
            (log_console_output ? "on" : "off") +
            "\n"
            "write_logs_to_files = " +
            (write_logs_to_files ? "on" : "off") +
            "\n"
            "write_logs_to_console = " +
            (write_logs_to_console ? "on" : "off") +
            "\n"
            "write_alerts_to_console = " +
            (write_alerts_to_console ? "on" : "off") +
            "\n"
            "component_logging_default = " +
            (component_logging_default ? "on" : "off") +
            "\n"
            "experimental = " +
            (experimental ? "on" : "off") +
            "\n";
    }

    [[nodiscard]]
    inline std::string ini_for_logger(
        const std::uint64_t merge_interval_seconds,
        const bool merge_logs_on_shutdown,
        const bool logging = true
    ) {
        if (merge_interval_seconds == logger_merge_interval_seconds &&
            merge_logs_on_shutdown == logger_merge_logs_on_shutdown &&
            logging) {
            return std::string {logger_ini};
        }
        return std::string {
            "[logger]\n"
            "merge_interval_seconds = " +
            std::to_string(merge_interval_seconds) +
            "\n"
            "merge_logs_on_shutdown = " +
            (merge_logs_on_shutdown ? "on" : "off") +
            "\nlogging = " +
            (logging ? "on" : "off") +
            "\n"
        };
    }

    [[nodiscard]]
    inline std::string ini_for_warnings(
        const bool missing_winkey_mapping,
        const bool unmapped_key
    ) {
        if (missing_winkey_mapping == warnings_missing_winkey_mapping &&
            unmapped_key == warnings_unmapped_key) {
            return std::string {warnings_ini};
        }
        return std::string {
            "[warnings]\n"
            "missing_winkey_mapping = "
        } + std::string {missing_winkey_mapping ? "on" : "off"} +
            "\n"
            "unmapped_key = " +
            std::string {unmapped_key ? "on" : "off"} +
            "\n";
    }

} // namespace ac::main::defaults
