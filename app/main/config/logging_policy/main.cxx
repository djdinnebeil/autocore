import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import components_editor_request;
import config_menu;
import logging_policy_config_detail;

import <fstream>;
import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace logging = ac::main::logging;
namespace menu = ac::config_menu;

namespace {

ac::Component logging_policy_config {
    "logging_policy_config",
    "auto_core"
};

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
        if (*level == "trace" || *level == "debug" ||
            *level == "info" || *level == "alert") {
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
    if (const auto interval = document->find(
            "log_merge", "merge_interval_seconds"
        )) {
        if (const auto parsed = parse_non_negative(*interval)) {
            values.merge_interval_seconds = static_cast<std::uint64_t>(*parsed);
        }
    }
    if (const auto shutdown_merge = document->find(
            "log_merge", "merge_logs_on_shutdown"
        )) {
        bool parsed = values.merge_logs_on_shutdown;
        if (parse_on_off(*shutdown_merge, parsed)) {
            values.merge_logs_on_shutdown = parsed;
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
    if (setting.key == "merge_interval_seconds") {
        return std::to_string(values.merge_interval_seconds);
    }
    if (setting.key == "merge_logs_on_shutdown") {
        return on_off(values.merge_logs_on_shutdown);
    }
    return on_off(values.component_logging_default);
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
    else if (setting.key == "merge_interval_seconds") {
        const auto parsed = parse_non_negative(value);
        if (!parsed) {
            return menu::ApplyResult::invalid;
        }
        values.merge_interval_seconds = static_cast<std::uint64_t>(*parsed);
    }
    else if (setting.key == "merge_logs_on_shutdown") {
        values.merge_logs_on_shutdown = value == "on";
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
    logging_policy_config.log_info("Logging Policy configuration opened");
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

[[nodiscard]]
std::string_view trim_view(const std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

[[nodiscard]]
bool read_file_bytes(
    const std::filesystem::path& path,
    std::string& text
) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    text.assign(
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    );
    return !input.bad();
}

[[nodiscard]]
bool logging_settings_preserved(
    const ac::ini::Document& before,
    const ac::ini::Document& after
) {
    for (const auto& [key, value] : before.settings("logging")) {
        if (key == "disable_all") {
            continue;
        }
        const auto current = after.find("logging", key);
        if (!current || *current != value) {
            return false;
        }
    }
    return true;
}

/**
 * Changes only `[logging] disable_all` to `on`.
 * Every other line is copied unchanged.
 */
[[nodiscard]]
std::string with_disable_all_on(const std::string& text) {
    std::string updated;
    updated.reserve(text.size() + 32);
    bool in_logging = false;
    bool replaced = false;
    std::size_t insert_at = std::string::npos;
    std::string insert_newline = "\n";

    std::size_t cursor = 0;
    while (cursor < text.size()) {
        const auto break_at = text.find_first_of("\r\n", cursor);
        const auto line_end =
            break_at == std::string::npos ? text.size() : break_at;
        const std::string_view line {
            text.data() + cursor,
            line_end - cursor
        };
        std::string_view ending;
        auto next = line_end;
        if (break_at != std::string::npos) {
            if (text[break_at] == '\r' &&
                break_at + 1 < text.size() &&
                text[break_at + 1] == '\n') {
                ending = std::string_view {text.data() + break_at, 2};
                next = break_at + 2;
            }
            else {
                ending = std::string_view {text.data() + break_at, 1};
                next = break_at + 1;
            }
        }

        const auto trimmed = trim_view(line);
        const bool comment =
            trimmed.starts_with('#') || trimmed.starts_with(';');
        const bool section = !comment &&
            trimmed.size() >= 2 &&
            trimmed.front() == '[' &&
            trimmed.back() == ']';
        if (section) {
            const auto name = trim_view(trimmed.substr(1, trimmed.size() - 2));
            in_logging = name == "logging";
            updated.append(line);
            updated.append(ending);
            if (in_logging) {
                insert_at = updated.size();
                insert_newline = ending.empty()
                    ? std::string {"\n"}
                    : std::string {ending};
            }
        }
        else if (
            in_logging &&
            !comment &&
            line.find('=') != std::string_view::npos &&
            trim_view(line.substr(0, line.find('='))) == "disable_all"
        ) {
            replaced = true;
            const auto equals = line.find('=');
            auto value_start = equals + 1;
            while (value_start < line.size() &&
                (line[value_start] == ' ' || line[value_start] == '\t')) {
                ++value_start;
            }
            updated.append(line.substr(0, value_start));
            updated.append("on");
            updated.append(ending);
        }
        else {
            updated.append(line);
            updated.append(ending);
        }
        cursor = next;
    }

    if (!replaced) {
        const auto inserted = std::string {"disable_all = on"} + insert_newline;
        if (insert_at == std::string::npos) {
            if (!updated.empty() && updated.back() != '\n') {
                updated.push_back('\n');
            }
            updated.append("[logging]\n");
            updated.append(inserted);
        }
        else {
            updated.insert(insert_at, inserted);
        }
    }
    return updated;
}

[[nodiscard]]
bool disable_all_is_on() {
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return false;
    }
    const auto value = document->find("logging", "disable_all");
    return value && *value == "on";
}

/**
 * Sets `disable_all = on` in an existing policy and checks the result.
 * Other logging settings must still parse to their previous values.
 * This function does not log.
 */
[[nodiscard]]
bool disable_existing_policy() {
    const auto path = ini_path();
    const auto before = ac::ini::read(path);
    std::string text;
    if (!before || !read_file_bytes(path, text)) {
        std::cerr << "Failed to read " << path.string() << ".\n";
        return false;
    }
    if (!cfg::write_bytes(path, with_disable_all_on(text))) {
        std::cerr << "Failed to write " << path.string() << ".\n";
        return false;
    }
    const auto after = ac::ini::read(path);
    if (!after || !disable_all_is_on() ||
        !logging_settings_preserved(*before, *after)) {
        std::cerr << "Failed to write " << path.string() << ".\n";
        return false;
    }
    return true;
}

} // namespace

int run_logging_policy_config(int argc, char* argv[]) {
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
        if (present) {
            return disable_existing_policy() ? 0 : 1;
        }
        if (!write_values(logging::disabled_defaults()) || !disable_all_is_on()) {
            std::cerr << "Failed to write " << ini_path().string() << ".\n";
            return 1;
        }
        return 0;
    }
    if (action == logging::Action::seed) {
        if (!write_values(logging::compiled_defaults())) {
            std::cerr << "Failed to write " << ini_path().string() << ".\n";
            return 1;
        }
        logging_policy_config.log_info("Logging Policy configuration opened");
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
        logging_policy_config.log_info("Logging Policy configuration opened");
        req::log_configuration_missing(logging_policy_config);
        req::log_writing_defaults(logging_policy_config);
        logging_policy_config.log_debug("Wrote {}", ini_path().string());
        return run_configuration_menu();
    }

    logging_policy_config.log_info("Logging Policy configuration opened");
    req::log_config_request(logging_policy_config, *launch);
    if (launch->seed) {
        req::log_seed_skipped(logging_policy_config, "config/logging_policy.ini");
        return 0;
    }
    if (launch->init) {
        req::log_initialization_skipped(logging_policy_config, "config/logging_policy.ini");
        return 0;
    }
    return run_configuration_menu();
}
