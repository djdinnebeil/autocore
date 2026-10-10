import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import components_editor_request;
import config_menu;

import <charconv>;
import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;
namespace menu = ac::config_menu;

namespace {

ac::Component logger_config {
    "logger_config",
    ac::logging::config::LoggingScope {"logger"}
};

constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting settings[] {
    {
        .key = "merge_interval_seconds",
        .display_name = "Merge interval",
        .summary =
            "Seconds between periodic merges. 0 disables the periodic merge.",
        .default_value = "60",
        .constraint = "Enter 0 or a positive number of seconds.",
    },
    {
        .key = "merge_logs_on_shutdown",
        .display_name = "Merge logs on shutdown",
        .summary = "Merge main logs after the hosted Logger exits.",
        .default_value = "on",
        .choices = on_off,
    },
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging for the Logger component.",
        .default_value = "on",
        .choices = on_off,
    },
};

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
        menu::with_header(
            settings,
            defaults::ini_for_logger(
                values.merge_interval_seconds,
                values.merge_logs_on_shutdown,
                values.logging
            )
        )
    );
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    const auto values = current_values();
    if (setting.key == "merge_interval_seconds") {
        return std::to_string(values.merge_interval_seconds);
    }
    if (setting.key == "merge_logs_on_shutdown") {
        return values.merge_logs_on_shutdown ? "on" : "off";
    }
    return values.logging ? "on" : "off";
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    auto values = current_values();
    if (setting.key == "merge_interval_seconds") {
        const auto parsed = parse_interval(value);
        if (!parsed) {
            return menu::ApplyResult::invalid;
        }
        values.merge_interval_seconds = *parsed;
    }
    else if (setting.key == "merge_logs_on_shutdown") {
        values.merge_logs_on_shutdown = value == "on";
    }
    else {
        values.logging = value == "on";
    }
    if (!write_values(values)) {
        logger_config.log_print("Failed to write {}.", ini_path().string());
        return menu::ApplyResult::failed;
    }
    logger_config.log_print("Wrote {}", ini_path().string());
    return menu::ApplyResult::stored;
}

[[nodiscard]]
int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Logger Configuration",
        settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
}

[[nodiscard]]
int write_missing_defaults() {
    if (!write_values({})) {
        logger_config.log_print("Failed to write {}.", ini_path().string());
        return 1;
    }
    logger_config.log_print("Wrote {}", ini_path().string());
    return 0;
}

[[nodiscard]]
int initialize_missing() {
    if (write_missing_defaults() != 0) {
        return 1;
    }
    const auto offer = menu::offer_configuration(
        "Logger Configuration",
        settings,
        current_text,
        std::cin,
        std::cout
    );
    if (offer == menu::OfferResult::failed) {
        return 1;
    }
    if (offer == menu::OfferResult::configure) {
        return run_configuration_menu();
    }
    return 0;
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
        if (write_missing_defaults() != 0) {
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
        req::log_writing_defaults(logger_config);
        if (initialize_missing() != 0) {
            return 1;
        }
        req::log_configuration_initialized(logger_config);
        return 0;
    }

    if (!present) {
        req::log_configuration_missing(logger_config);
        req::log_writing_defaults(logger_config);
        if (write_missing_defaults() != 0) {
            return 1;
        }
    }
    return run_configuration_menu();
}
