import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;

import <charconv>;
import <iostream>;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;

namespace {

ac::Component logger_config {"logger_config"};

void say_line(const std::string_view line) {
    std::cout << line << '\n';
    logger_config.log("{}", line);
}

void log_choice(const std::string_view name, const std::string_view value) {
    logger_config.log("{} = {}", name, value);
}

struct LoggerValues {
    std::string directory {defaults::logger_directory};
    std::uint64_t merge_interval_seconds {
        defaults::logger_merge_interval_seconds
    };
    bool merge_logs_on_shutdown {defaults::logger_merge_logs_on_shutdown};
    bool write_logs_to_console {defaults::logger_write_logs_to_console};
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
    if (const auto directory = document->find("logger", "directory")) {
        if (!directory->empty()) {
            values.directory = std::string {*directory};
        }
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
        if (*merge == "on" || *merge == "true") {
            values.merge_logs_on_shutdown = true;
        }
        else if (*merge == "off" || *merge == "false") {
            values.merge_logs_on_shutdown = false;
        }
    }
    if (const auto console = document->find(
            "logger", "write_logs_to_console"
        )) {
        if (*console == "on" || *console == "true") {
            values.write_logs_to_console = true;
        }
        else if (*console == "off" || *console == "false") {
            values.write_logs_to_console = false;
        }
    }
    return values;
}

[[nodiscard]]
bool write_values(const LoggerValues& values) {
    return cfg::write_bytes(
        ini_path(),
        defaults::ini_for_logger(
            values.directory,
            values.merge_interval_seconds,
            values.merge_logs_on_shutdown,
            values.write_logs_to_console
        )
    );
}

void show_settings() {
    const auto values = current_values();
    say_line("Current config/logger.ini");
    say_line(std::string {"directory = "} + values.directory);
    say_line(
        std::string {"merge_interval_seconds = "} +
        std::to_string(values.merge_interval_seconds)
    );
    say_line(
        std::string {"merge_logs_on_shutdown = "} +
        (values.merge_logs_on_shutdown ? "on" : "off")
    );
    say_line(
        std::string {"write_logs_to_console = "} +
        (values.write_logs_to_console ? "on" : "off")
    );
}

[[nodiscard]]
std::optional<LoggerValues> prompt_values(const LoggerValues& suggestion) {
    LoggerValues values = suggestion;
    const auto directory = cfg::prompt_text(
        "directory",
        suggestion.directory
    );
    if (!directory) {
        return std::nullopt;
    }
    values.directory = directory->empty()
        ? std::string {defaults::logger_directory}
        : *directory;
    log_choice("directory", values.directory);

    while (true) {
        const auto interval = cfg::prompt_text(
            "merge_interval_seconds",
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

    const auto shutdown_merge = cfg::prompt_on_off(
        "merge_logs_on_shutdown",
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

    const auto console = cfg::prompt_on_off(
        "write_logs_to_console",
        suggestion.write_logs_to_console
    );
    if (!console) {
        return std::nullopt;
    }
    values.write_logs_to_console = *console;
    log_choice(
        "write_logs_to_console",
        values.write_logs_to_console ? "on" : "off"
    );
    return values;
}

[[nodiscard]]
int first_time() {
    say_line("config/logger.ini is missing. Create it using the defaults.");
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

int main() {
    logger_config.log_main("logger_config.exe started");

    std::error_code error;
    if (std::filesystem::exists(ini_path(), error)) {
        return configuration_mode();
    }
    if (error) {
        logger_config.log_print(
            "Failed to inspect {}",
            ini_path().string()
        );
        return 1;
    }
    return first_time();
}
