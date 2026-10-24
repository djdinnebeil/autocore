import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import components_editor_request;
import config_menu;
import logging_policy_config_detail;

import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace logging = ac::main::logging;
namespace menu = ac::config_menu;

namespace {

ac::Component logging_policy_config {"logging_policy_config"};

using LoggingValues = logging::Values;

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "logging_policy.ini";
}

[[nodiscard]]
bool parse_on_off(const std::string_view text, bool& value) {
    if (text == "on") {
        value = true;
        return true;
    }
    if (text == "off") {
        value = false;
        return true;
    }
    return false;
}

[[nodiscard]]
std::optional<std::int64_t> parse_non_negative(const std::string_view text) {
    std::int64_t parsed = 0;
    const auto result = std::from_chars(
        text.data(),
        text.data() + text.size(),
        parsed
    );
    if (result.ec != std::errc {} ||
        result.ptr != text.data() + text.size() ||
        parsed < 0) {
        return std::nullopt;
    }
    return parsed;
}

[[nodiscard]]
LoggingValues current_values() {
    LoggingValues values;
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return values;
    }
    if (const auto disable_all = document->find("logging", "disable_all")) {
        bool parsed = values.disable_all;
        if (parse_on_off(*disable_all, parsed)) {
            values.disable_all = parsed;
        }
    }
    if (const auto directory = document->find("logging", "directory")) {
        if (!directory->empty()) {
            values.directory = std::string {*directory};
        }
    }
    if (const auto cap = document->find("logging", "max_directory_mb")) {
        if (const auto parsed = parse_non_negative(*cap)) {
            values.max_directory_mb = *parsed;
        }
    }
    if (const auto retention = document->find("logging", "retention_days")) {
        if (const auto parsed = parse_non_negative(*retention)) {
            values.retention_days = *parsed;
        }
    }
    if (const auto level = document->find("logging", "level")) {
        if (*level == "debug" || *level == "info" || *level == "alert") {
            values.level = std::string {*level};
        }
    }
    if (const auto console_output = document->find(
            "logging", "log_console_output"
        )) {
        bool parsed = values.log_console_output;
        if (parse_on_off(*console_output, parsed)) {
            values.log_console_output = parsed;
        }
    }
    if (const auto files = document->find("logging", "write_logs_to_files")) {
        bool parsed = values.write_logs_to_files;
        if (parse_on_off(*files, parsed)) {
            values.write_logs_to_files = parsed;
        }
    }
    if (const auto console = document->find(
            "logging", "write_logs_to_console"
        )) {
        bool parsed = values.write_logs_to_console;
        if (parse_on_off(*console, parsed)) {
            values.write_logs_to_console = parsed;
        }
    }
    if (const auto alerts = document->find(
            "logging", "write_alerts_to_console"
        )) {
        bool parsed = values.write_alerts_to_console;
        if (parse_on_off(*alerts, parsed)) {
            values.write_alerts_to_console = parsed;
        }
    }
    if (const auto family = document->find(
            "logging", "component_logging_default"
        )) {
        bool parsed = values.component_logging_default;
        if (parse_on_off(*family, parsed)) {
            values.component_logging_default = parsed;
        }
    }
    if (const auto experimental = document->find("logging", "experimental")) {
        bool parsed = values.experimental;
        if (parse_on_off(*experimental, parsed)) {
            values.experimental = parsed;
        }
    }
    return values;
}

[[nodiscard]]
bool write_values(const LoggingValues& values) {
    return cfg::write_bytes(ini_path(), logging::ini_text(values));
}

[[nodiscard]]
std::string on_off(const bool value) {
    return value ? "on" : "off";
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    const auto values = current_values();
    if (setting.key == "disable_all") {
        return on_off(values.disable_all);
    }
    if (setting.key == "directory") {
        return values.directory;
    }
    if (setting.key == "max_directory_mb") {
        return std::to_string(values.max_directory_mb);
    }
    if (setting.key == "retention_days") {
        return std::to_string(values.retention_days);
    }
    if (setting.key == "level") {
        return values.level;
    }
    if (setting.key == "log_console_output") {
        return on_off(values.log_console_output);
    }
    if (setting.key == "write_logs_to_files") {
        return on_off(values.write_logs_to_files);
    }
    if (setting.key == "write_logs_to_console") {
        return on_off(values.write_logs_to_console);
    }
    if (setting.key == "write_alerts_to_console") {
        return on_off(values.write_alerts_to_console);
    }
    if (setting.key == "component_logging_default") {
        return on_off(values.component_logging_default);
    }
    return on_off(values.experimental);
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    auto values = current_values();
    if (setting.key == "disable_all") {
        values.disable_all = value == "on";
    }
    else if (setting.key == "directory") {
        values.directory = value.empty()
            ? ac::main::defaults::logging_directory_text
            : std::string {value};
    }
    else if (setting.key == "max_directory_mb") {
        const auto parsed = parse_non_negative(value);
        if (!parsed) {
            return menu::ApplyResult::invalid;
        }
        values.max_directory_mb = *parsed;
    }
    else if (setting.key == "retention_days") {
        const auto parsed = parse_non_negative(value);
        if (!parsed) {
            return menu::ApplyResult::invalid;
        }
        values.retention_days = *parsed;
    }
    else if (setting.key == "level") {
        values.level = std::string {value};
    }
    else if (setting.key == "log_console_output") {
        values.log_console_output = value == "on";
    }
    else if (setting.key == "write_logs_to_files") {
        values.write_logs_to_files = value == "on";
    }
    else if (setting.key == "write_logs_to_console") {
        values.write_logs_to_console = value == "on";
    }
    else if (setting.key == "write_alerts_to_console") {
        values.write_alerts_to_console = value == "on";
    }
    else if (setting.key == "component_logging_default") {
        values.component_logging_default = value == "on";
    }
    else {
        values.experimental = value == "on";
    }
    if (!write_values(values)) {
        logging_policy_config.print("Failed to write {}.", ini_path().string());
        return menu::ApplyResult::failed;
    }
    logging_policy_config.log_debug("Wrote {}", ini_path().string());
    return menu::ApplyResult::stored;
}

[[nodiscard]]
int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Logging Configuration",
        logging::menu_settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
}

[[nodiscard]]
int initialize_missing() {
    if (!write_values(logging::compiled_defaults())) {
        std::cerr << "Failed to write " << ini_path().string() << ".\n";
        return 1;
    }
    logging_policy_config.log_info("logging_policy_config.exe started");
    ac::config::components_request::log_configuration_missing(logging_policy_config);
    ac::config::components_request::log_writing_defaults(logging_policy_config);
    logging_policy_config.log_debug("Wrote {}", ini_path().string());
    const auto result = menu::prompt_settings(
        logging::menu_settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout,
        "Logging Configuration"
    );
    return result.ok ? 0 : 1;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    bool disable_requested = false;
    std::vector<char*> filtered;
    filtered.reserve(static_cast<std::size_t>(argc));
    filtered.push_back(argv[0]);
    for (int index = 1; index < argc; ++index) {
        if (std::string_view {argv[index]} == "--disable") {
            disable_requested = true;
            continue;
        }
        filtered.push_back(argv[index]);
    }
    const auto launch =
        ac::config::components_request::parse_config_launch(
            static_cast<int>(filtered.size()),
            filtered.data()
        );
    if (!launch) {
        return 1;
    }

    std::error_code error;
    const bool present = std::filesystem::exists(ini_path(), error);
    if (error) {
        std::cerr << "Failed to inspect " << ini_path().string() << ".\n";
        return 1;
    }

    namespace req = ac::config::components_request;
    const auto action = logging::select_action(
        present,
        launch->init,
        launch->seed,
        disable_requested
    );
    if (action == logging::Action::disable) {
        if (!write_values(logging::disabled_defaults())) {
            std::cerr << "Failed to write " << ini_path().string() << ".\n";
            return 1;
        }
        logging_policy_config.log_info("logging_policy_config.exe started");
        req::log_configuration_initialized(logging_policy_config);
        return 0;
    }
    if (action == logging::Action::seed) {
        if (!write_values(logging::compiled_defaults())) {
            std::cerr << "Failed to write " << ini_path().string() << ".\n";
            return 1;
        }
        logging_policy_config.log_info("logging_policy_config.exe started");
        req::log_writing_defaults(logging_policy_config);
        req::log_configuration_initialized(logging_policy_config);
        return 0;
    }
    if (action == logging::Action::initialize) {
        if (initialize_missing() != 0) {
            return 1;
        }
        req::log_configuration_initialized(logging_policy_config);
        return 0;
    }
    if (action == logging::Action::create_missing) {
        if (!write_values(logging::compiled_defaults())) {
            std::cerr << "Failed to write " << ini_path().string() << ".\n";
            return 1;
        }
        logging_policy_config.log_info("logging_policy_config.exe started");
        req::log_configuration_missing(logging_policy_config);
        req::log_writing_defaults(logging_policy_config);
        logging_policy_config.log_debug("Wrote {}", ini_path().string());
        return run_configuration_menu();
    }

    logging_policy_config.log_info("logging_policy_config.exe started");
    req::log_config_request(logging_policy_config, *launch);
    if (launch->seed) {
        req::log_seed_skipped(logging_policy_config, "config/logging_policy.ini");
        return 0;
    }
    if (disable_requested) {
        logging_policy_config.log_debug(
            "Disable skipped; config/logging_policy.ini already exists"
        );
        return 0;
    }
    if (launch->init) {
        req::log_initialization_skipped(logging_policy_config, "config/logging_policy.ini");
        return 0;
    }
    return run_configuration_menu();
}
