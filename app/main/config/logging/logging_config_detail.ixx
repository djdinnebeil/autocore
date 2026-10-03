/**
 * \file logging_config_detail.ixx
 * \brief Launch selection, initialization menu, and prompts for logging.ini.
 *
 * `--seed` takes precedence over `--init`. Prompt order matches the
 * serialized key order. Validation matches the existing on/off and
 * log/print rules.
 */
export module logging_config_detail;

import std;
import auto_core.main.defaults;

namespace defaults = ac::main::defaults;

namespace {

    [[nodiscard]] std::string trim(const std::string_view value) {
        const auto first = value.find_first_not_of(" \t");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t");
        return std::string {value.substr(first, last - first + 1)};
    }

    [[nodiscard]] std::optional<std::string> read_line(std::istream& input) {
        std::string line;
        if (!std::getline(input, line)) {
            return std::nullopt;
        }
        return trim(line);
    }

    [[nodiscard]] std::optional<std::string> prompt_text(
        std::istream& input,
        std::ostream& output,
        const std::string_view label,
        const std::string_view suggestion
    ) {
        output << label << " [" << suggestion << "]: ";
        output.flush();
        const auto line = read_line(input);
        if (!line) {
            return std::nullopt;
        }
        if (line->empty()) {
            return std::string {suggestion};
        }
        return *line;
    }

    [[nodiscard]] std::optional<bool> prompt_on_off(
        std::istream& input,
        std::ostream& output,
        const std::string_view label,
        const bool suggestion
    ) {
        const auto suggested = suggestion ? "on" : "off";
        const auto line = prompt_text(input, output, label, suggested);
        if (!line) {
            return std::nullopt;
        }
        std::string lowered {*line};
        for (char& character : lowered) {
            if (character >= 'A' && character <= 'Z') {
                character = static_cast<char>(character - 'A' + 'a');
            }
        }
        if (lowered == "on" || lowered == "off") {
            return lowered == "on";
        }
        output
            << "Enter on or off. Using "
            << *line
            << " is invalid; keeping "
            << suggested
            << ".\n";
        return suggestion;
    }

    [[nodiscard]] std::optional<std::string> prompt_log_print_mode(
        std::istream& input,
        std::ostream& output,
        const std::string_view current
    ) {
        while (true) {
            output << "log_print_mode [" << current << "]: ";
            output.flush();
            const auto line = read_line(input);
            if (!line) {
                return std::nullopt;
            }
            if (line->empty()) {
                return std::string {current};
            }
            if (*line == "log" || *line == "print") {
                return *line;
            }
            output << "Enter log or print.\n";
        }
    }

} // namespace

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

    /**
     * \brief Selects the owner action.
     *
     * `--disable` is the noninteractive disable-all write. `--seed` takes
     * precedence over `--init`. An existing file is preserved.
     */
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
        return defaults::ini_for_logging(
            values.disable_all,
            values.write_logs_to_files,
            values.directory,
            values.write_logs_to_console,
            values.log_print_mode,
            values.component_logging_default
        );
    }

    [[nodiscard]] std::optional<Values> prompt_values(
        std::istream& input,
        std::ostream& output,
        const Values& suggestion
    ) {
        Values values = suggestion;

        const auto disable_all = prompt_on_off(
            input,
            output,
            "disable_all",
            suggestion.disable_all
        );
        if (!disable_all) {
            return std::nullopt;
        }
        values.disable_all = *disable_all;

        const auto directory = prompt_text(
            input,
            output,
            "directory",
            suggestion.directory
        );
        if (!directory) {
            return std::nullopt;
        }
        values.directory = directory->empty()
            ? std::string {defaults::logging_directory}
            : *directory;

        const auto files = prompt_on_off(
            input,
            output,
            "write_logs_to_files",
            suggestion.write_logs_to_files
        );
        if (!files) {
            return std::nullopt;
        }
        values.write_logs_to_files = *files;

        const auto console = prompt_on_off(
            input,
            output,
            "write_logs_to_console",
            suggestion.write_logs_to_console
        );
        if (!console) {
            return std::nullopt;
        }
        values.write_logs_to_console = *console;

        const auto mode = prompt_log_print_mode(
            input,
            output,
            suggestion.log_print_mode
        );
        if (!mode) {
            return std::nullopt;
        }
        values.log_print_mode = *mode;

        const auto family = prompt_on_off(
            input,
            output,
            "component_logging_default",
            suggestion.component_logging_default
        );
        if (!family) {
            return std::nullopt;
        }
        values.component_logging_default = *family;
        return values;
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
        if (!write(defaults::logging_ini)) {
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
