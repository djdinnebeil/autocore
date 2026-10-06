import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import components_editor_request;

import <charconv>;
import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;

namespace {

ac::Component logger_config {
    "logger_config",
    ac::logging::config::LoggingScope {"logger"}
};

void say_line(const std::string_view line) {
    std::cout << line << '\n';
    logger_config.log("{}", line);
}

void log_choice(const std::string_view name, const std::string_view value) {
    logger_config.log("{} = {}", name, value);
}

struct LoggerValues {
    std::uint64_t merge_interval_seconds {
        defaults::logger_merge_interval_seconds
    };
    bool merge_logs_on_shutdown {defaults::logger_merge_logs_on_shutdown};
    bool logging = true;
};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "logger.ini";
}

[[nodiscard]]
std::optional<std::uint64_t> parse_interval(const std::string_view text) {
    if (text.empty()) {
        return std::nullopt;
    }
    std::int64_t parsed = 0;
    const auto* const end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, parsed);
    if (result.ec != std::errc {} || result.ptr != end || parsed < 0) {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(parsed);
}

[[nodiscard]]
LoggerValues current_values() {
    LoggerValues values;
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return values;
    }
    if (const auto interval = document->find(
            "logger", "merge_interval_seconds"
        )) {
        if (const auto parsed = parse_interval(*interval)) {
            values.merge_interval_seconds = *parsed;
        }
    }
    if (const auto merge = document->find(
            "logger", "merge_logs_on_shutdown"
        )) {
        if (*merge == "on") {
            values.merge_logs_on_shutdown = true;
        }
        else if (*merge == "off") {
            values.merge_logs_on_shutdown = false;
        }
    }
    bool logging_set = false;
    if (const auto logging = document->find("logger", "logging")) {
        if (*logging == "on") {
            values.logging = true;
            logging_set = true;
        }
        else if (*logging == "off") {
            values.logging = false;
            logging_set = true;
        }
    }
    if (!logging_set) {
        values.logging = ac::logging::config::component_logging_default();
    }
    return values;
}

[[nodiscard]]
bool write_values(const LoggerValues& values) {
    return cfg::write_bytes(
        ini_path(),
        defaults::ini_for_logger(
            values.merge_interval_seconds,
            values.merge_logs_on_shutdown,
            values.logging
        )
    );
}

void show_settings() {
    const auto values = current_values();
    say_line("Current config/logger.ini");
    say_line(
        std::string {"merge_interval_seconds = "} +
        std::to_string(values.merge_interval_seconds)
    );
    say_line(
        std::string {"merge_logs_on_shutdown = "} +
        (values.merge_logs_on_shutdown ? "on" : "off")
    );
    say_line(
        std::string {"logging = "} +
        (values.logging ? "on" : "off")
    );
}

[[nodiscard]]
std::string lowered(std::string text) {
    for (char& character : text) {
        if (character >= 'A' && character <= 'Z') {
            character = static_cast<char>(character - 'A' + 'a');
        }
    }
    return text;
}

[[nodiscard]]
std::optional<bool> prompt_shutdown_merge(const bool suggestion) {
    while (true) {
        std::cout << "Merge main logs when Auto Core shuts down? ["
                  << (suggestion ? "Y/n" : "y/N") << "]: ";
        const auto line = cfg::read_line();
        if (!line) {
            return std::nullopt;
        }
        logger_config.log(
            "Merge main logs when Auto Core shuts down? {}",
            *line
        );
        if (line->empty()) {
            return suggestion;
        }
        const auto answer = lowered(*line);
        if (answer == "y" || answer == "yes") {
            return true;
        }
        if (answer == "n" || answer == "no") {
            return false;
        }
        say_line("Enter Y or N.");
    }
}

[[nodiscard]]
std::optional<LoggerValues> prompt_values(const LoggerValues& suggestion) {
    LoggerValues values = suggestion;
    say_line("0 disables periodic merging.");
    while (true) {
        const auto interval = cfg::prompt_text(
            "Periodic merge interval in seconds",
            std::to_string(suggestion.merge_interval_seconds)
        );
        if (!interval) {
            return std::nullopt;
        }
        const auto parsed = parse_interval(*interval);
        if (!parsed) {
            say_line("Enter 0 or a positive number of seconds.");
            continue;
        }
        values.merge_interval_seconds = *parsed;
        log_choice(
            "merge_interval_seconds",
            std::to_string(*parsed)
        );
        break;
    }

    const auto shutdown_merge = prompt_shutdown_merge(
        suggestion.merge_logs_on_shutdown
    );
    if (!shutdown_merge) {
        return std::nullopt;
    }
    values.merge_logs_on_shutdown = *shutdown_merge;
    log_choice(
        "merge_logs_on_shutdown",
        values.merge_logs_on_shutdown ? "on" : "off"
    );
    std::optional<bool> logging;
    while (true) {
        std::cout << "Enable logging? ["
                  << (suggestion.logging ? "Y/n" : "y/N")
                  << "]: ";
        const auto line = cfg::read_line();
        if (!line) {
            return std::nullopt;
        }
        if (line->empty()) {
            logging = suggestion.logging;
            break;
        }
        if (*line == "y" || *line == "Y") {
            logging = true;
            break;
        }
        if (*line == "n" || *line == "N") {
            logging = false;
            break;
        }
        say_line("Enter Y or n.");
    }
    values.logging = *logging;
    log_choice("logging", values.logging ? "on" : "off");
    return values;
}

[[nodiscard]]
int first_time() {
    say_line("config/logger.ini is missing. Configure Logger.");
    std::cout << '\n';
    const auto values = prompt_values({});
    if (!values) {
        return 1;
    }
    if (!write_values(*values)) {
        logger_config.log_print(
            "Failed to write {}.",
            ini_path().string()
        );
        return 1;
    }
    logger_config.log_print("Wrote {}", ini_path().string());
    return 0;
}

int configuration_mode() {
    show_settings();
    while (true) {
        std::cout << '\n';
        say_line("logger_config");
        say_line("1. Modify settings");
        say_line("2. Restore defaults");
        say_line("3. Exit");
        std::cout << "Choice: ";
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        logger_config.log("Choice: {}", *line);
        if (*line == "1") {
            const auto values = prompt_values(current_values());
            if (!values) {
                return 1;
            }
            if (!write_values(*values)) {
                logger_config.log_print(
                    "Failed to write {}.",
                    ini_path().string()
                );
                return 1;
            }
            logger_config.log_print("Wrote {}", ini_path().string());
            show_settings();
        }
        else if (*line == "2") {
            if (!write_values({})) {
                logger_config.log_print(
                    "Failed to restore defaults in {}.",
                    ini_path().string()
                );
                return 1;
            }
            logger_config.log_print(
                "Restored defaults in {}",
                ini_path().string()
            );
            show_settings();
        }
        else if (*line == "3" || line->empty()) {
            return 0;
        }
        else {
            say_line("Enter 1, 2, or 3.");
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    logger_config.log_main("logger_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    std::error_code error;
    const bool present = std::filesystem::exists(ini_path(), error);
    if (error) {
        logger_config.log_print(
            "Failed to inspect {}",
            ini_path().string()
        );
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(logger_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(logger_config, "config/logger.ini");
            return 0;
        }
        req::log_writing_defaults(logger_config);
        if (!write_values({})) {
            logger_config.log_print(
                "Failed to write {}.",
                ini_path().string()
            );
            return 1;
        }
        req::log_configuration_initialized(logger_config);
        return 0;
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(logger_config, "config/logger.ini");
            return 0;
        }
        req::log_configuration_missing(logger_config);
        return first_time();
    }

    if (present) {
        return configuration_mode();
    }
    return first_time();
}
