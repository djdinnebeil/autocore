/**
 * \file logging_config_detail.ixx
 * \brief Launch selection and INI text for logging.ini.
 *
 * `--disable` is the noninteractive disable-all write. `--seed` takes
 * precedence over `--init`. An existing file is preserved.
 */
export module logging_config_detail;

import std;
import auto_core.main.defaults;
import config_menu;

namespace defaults = ac::main::defaults;

export namespace ac::main::logging {

    enum class Action {
        configure,
        create_missing,
        initialize,
        seed,
        disable,
        skip_initialization,
        skip_seed,
        skip_disable
    };

    struct Values {
        bool disable_all {defaults::logging_disable_all};
        std::string directory {defaults::logging_directory};
        bool write_logs_to_files {defaults::logging_write_logs_to_files};
        bool write_logs_to_console {defaults::logging_write_logs_to_console};
        std::string log_print_mode {defaults::logging_log_print_mode};
        bool component_logging_default {
            defaults::logging_component_logging_default
        };
    };

    struct CommitResult {
        int code {0};
        bool reported_success {false};
    };

    inline constexpr std::string_view on_off[] {"on", "off"};
    inline constexpr std::string_view log_or_print[] {"log", "print"};

    inline const ac::config_menu::Setting menu_settings[] {
        {
            .key = "disable_all",
            .display_name = "Disable all logging",
            .summary =
                "Stops logging-controlled file and console output. Menus and "
                "user-facing messages stay on the console.",
            .default_value = "off",
            .choices = on_off,
        },
        {
            .key = "directory",
            .display_name = "Directory",
            .summary =
                "Log directory. A relative path resolves against the "
                "installation root.",
            .default_value = defaults::logging_directory,
            .constraint = "Enter a relative or absolute directory.",
        },
        {
            .key = "write_logs_to_files",
            .display_name = "Write logs to files",
            .summary = "Write log records to files when logging is enabled.",
            .default_value = "on",
            .choices = on_off,
        },
        {
            .key = "write_logs_to_console",
            .display_name = "Write logs to console",
            .summary = "Also send ordinary log records to the console.",
            .default_value = "off",
            .choices = on_off,
        },
        {
            .key = "log_print_mode",
            .display_name = "Log print mode",
            .summary =
                "print keeps log_print on the console. log treats it as a "
                "logging record.",
            .default_value = defaults::logging_log_print_mode,
            .choices = log_or_print,
        },
        {
            .key = "component_logging_default",
            .display_name = "Component logging default",
            .summary =
                "Fallback when a component logging key is missing or invalid.",
            .default_value = "on",
            .choices = on_off,
        },
    };

    [[nodiscard]] Action select_action(
        const bool configuration_exists,
        const bool init_requested,
        const bool seed_requested,
        const bool disable_requested = false
    ) noexcept {
        if (disable_requested) {
            return configuration_exists ? Action::skip_disable : Action::disable;
        }
        if (seed_requested) {
            return configuration_exists ? Action::skip_seed : Action::seed;
        }
        if (init_requested) {
            return configuration_exists
                ? Action::skip_initialization
                : Action::initialize;
        }
        return configuration_exists ? Action::configure : Action::create_missing;
    }

    [[nodiscard]] Values compiled_defaults() {
        return {};
    }

    [[nodiscard]] Values disabled_defaults() {
        Values values;
        values.disable_all = true;
        return values;
    }

    [[nodiscard]] std::string ini_text(const Values& values) {
        return ac::config_menu::with_header(
            menu_settings,
            defaults::ini_for_logging(
                values.disable_all,
                values.write_logs_to_files,
                values.directory,
                values.write_logs_to_console,
                values.log_print_mode,
                values.component_logging_default
            )
        );
    }

    /**
     * \brief Writes seed text only when the file is missing.
     *
     * A failed write returns nonzero and does not report success.
     */
    [[nodiscard]] CommitResult commit_seed(
        const bool configuration_exists,
        const std::function<bool(std::string_view)>& write
    ) {
        if (configuration_exists) {
            return {};
        }
        if (!write(ini_text(compiled_defaults()))) {
            return {.code = 1, .reported_success = false};
        }
        return {.code = 0, .reported_success = true};
    }

    /**
     * \brief Writes the compiled file with `disable_all = on` when missing.
     *
     * An existing file is not rewritten. A failed write does not report success.
     */
    [[nodiscard]] CommitResult commit_disable(
        const bool configuration_exists,
        const std::function<bool(std::string_view)>& write
    ) {
        if (configuration_exists) {
            return {};
        }
        if (!write(ini_text(disabled_defaults()))) {
            return {.code = 1, .reported_success = false};
        }
        return {.code = 0, .reported_success = true};
    }

    [[nodiscard]] CommitResult commit_text(
        const std::string_view text,
        const std::function<bool(std::string_view)>& write
    ) {
        if (!write(text)) {
            return {.code = 1, .reported_success = false};
        }
        return {.code = 0, .reported_success = true};
    }

} // namespace ac::main::logging
