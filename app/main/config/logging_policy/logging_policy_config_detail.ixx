/**
 * \file logging_policy_config_detail.ixx
 * \brief Launch selection and INI text for logging_policy.ini.
 *
 * `--disable` sets `disable_all = on`. An existing file changes only that
 * key. `--seed` takes precedence over `--init` when disable is not
 * requested. Seed and init leave an existing file unchanged.
 */
export module logging_policy_config_detail;

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
        std::string directory {defaults::logging_directory_text};
        std::int64_t max_directory_mb {defaults::logging_max_directory_mb};
        std::int64_t retention_days {defaults::logging_retention_days};
        std::string level {defaults::logging_level};
        bool log_console_output {defaults::logging_log_console_output};
        bool write_logs_to_files {defaults::logging_write_logs_to_files};
        bool write_logs_to_console {defaults::logging_write_logs_to_console};
        bool write_alerts_to_console {
            defaults::logging_write_alerts_to_console
        };
        bool component_logging_default {
            defaults::logging_component_logging_default
        };
        std::uint64_t merge_interval_seconds {5};
        bool merge_logs_on_shutdown {true};
    };

    struct CommitResult {
        int code {0};
        bool reported_success {false};
    };

    inline constexpr std::string_view on_off[] {"on", "off"};
    inline constexpr std::string_view levels[] {"trace", "debug", "info", "alert"};

    inline const ac::config_menu::Setting menu_settings[] {
        {
            .key = "disable_all",
            .display_name = "Disable all logging",
            .summary =
                "Suppresses diagnostic file and console output. print() "
                "stays on the console.",
            .default_value = "off",
            .choices = on_off,
        },
        {
            .key = "directory",
            .display_name = "Directory",
            .summary =
                "Log directory. A relative path resolves against the "
                "installation root.",
            .default_value = std::string_view {
                defaults::logging_directory_text
            },
            .constraint = "Enter a relative or absolute directory.",
            .directory_path = true,
        },
        {
            .key = "max_directory_mb",
            .display_name = "Maximum directory size",
            .summary =
                "Megabytes of .log files to keep. Oldest files are deleted "
                "first. A log named with today's local date is kept and "
                "still counts toward the total. 0 keeps those logs.",
            .default_value = "25",
            .constraint =
                "Enter a non-negative integer. 0 keeps logs named with "
                "today's local date.",
            .numeric_prompt = "Enter an integer",
        },
        {
            .key = "retention_days",
            .display_name = "Retention days",
            .summary =
                "Elapsed UTC days to keep, using each file's last write "
                "time. A log named with today's local date is kept. "
                "0 deletes every other .log file last written before now.",
            .default_value = "10",
            .constraint =
                "Enter a non-negative integer. 0 keeps logs named with "
                "today's local date.",
            .numeric_prompt = "Enter an integer",
        },
        {
            .key = "level",
            .display_name = "Level",
            .summary =
                "Minimum diagnostic severity. debug includes info and "
                "alert. info includes alert.",
            .default_value = defaults::logging_level,
            .choices = levels,
        },
        {
            .key = "log_console_output",
            .display_name = "Log console output",
            .summary =
                "Allow print() and printnl() to participate in the "
                "component file sink. Does not control the console write.",
            .default_value = "on",
            .choices = on_off,
        },
        {
            .key = "write_logs_to_files",
            .display_name = "Write logs to files",
            .summary =
                "Master file-output gate for component logging, including "
                "eligible print() and printnl() copies.",
            .default_value = "on",
            .choices = on_off,
        },
        {
            .key = "write_logs_to_console",
            .display_name = "Write logs to console",
            .summary = "Mirror enabled diagnostic logs to the console.",
            .default_value = "off",
            .choices = on_off,
        },
        {
            .key = "write_alerts_to_console",
            .display_name = "Write alerts to console",
            .summary =
                "Show enabled alerts on the console. An alert already "
                "mirrored to the console is written once.",
            .default_value = "on",
            .choices = on_off,
        },
        {
            .key = "component_logging_default",
            .display_name = "Component logging default",
            .summary =
                "Fallback when a component logging key is missing or invalid.",
            .default_value = "on",
            .choices = on_off,
        },
        {
            .key = "merge_interval_seconds",
            .display_name = "Merge interval",
            .summary =
                "Seconds between Main log merges when log_merger is on. "
                "0 turns the periodic merge off. The shutdown merge is a "
                "separate setting.",
            .default_value = "5",
            .constraint =
                "Enter a non-negative integer. 0 disables periodic merging.",
            .numeric_prompt = "Enter an integer",
        },
        {
            .key = "merge_logs_on_shutdown",
            .display_name = "Merge logs on shutdown",
            .summary =
                "After other components stop, run one more merge when "
                "log_merger is on. This is independent of the interval.",
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
            return Action::disable;
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
        std::size_t logging_settings = 0;
        for (const auto& setting : menu_settings) {
            if (setting.key == "merge_interval_seconds") {
                break;
            }
            ++logging_settings;
        }
        auto header = ac::config_menu::reference_header(
            std::span {menu_settings, logging_settings}
        );
        if (header.ends_with("\n\n")) {
            header.pop_back();
        }
        auto text = header + defaults::ini_for_logging(
            values.disable_all,
            values.directory,
            values.max_directory_mb,
            values.retention_days,
            values.level,
            values.log_console_output,
            values.write_logs_to_files,
            values.write_logs_to_console,
            values.write_alerts_to_console,
            values.component_logging_default
        );
        constexpr std::string_view disable_all =
            "# disable_all = on | off\n";
        constexpr std::string_view directory_notes =
            "# directory = relative or absolute path\n"
            "# max_directory_mb = a non-negative integer in megabytes. "
            "0 keeps logs named with today's local date.\n"
            "# retention_days = elapsed UTC days, from last write time. "
            "0 keeps logs named with today's local date.\n";
        if (text.starts_with(disable_all)) {
            text.insert(disable_all.size(), directory_notes);
        }
        if (text.empty() || text.back() != '\n') {
            text.push_back('\n');
        }
        text.push_back('\n');
        text +=
            "# merge_interval_seconds = a positive integer in seconds. "
            "0 disables periodic merging. Applies when components.list "
            "enables log_merger.\n"
            "# merge_logs_on_shutdown = on | off. Applies independently of "
            "the periodic merge interval, and only when log_merger is on.\n"
            "[log_merge]\n"
            "merge_interval_seconds = ";
        text += std::to_string(values.merge_interval_seconds);
        text += "\nmerge_logs_on_shutdown = ";
        text += values.merge_logs_on_shutdown ? "on" : "off";
        text += "\n";
        return text;
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
