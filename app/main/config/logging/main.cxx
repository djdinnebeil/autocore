import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import components_editor_request;
import logging_config_detail;

import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;
namespace logging = ac::main::logging;

namespace {

ac::Component logging_config {"logging_config"};

using LoggingValues = logging::Values;

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "logging.ini";
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
    if (const auto files = document->find("logging", "write_logs_to_files")) {
        bool parsed = values.write_logs_to_files;
        if (parse_on_off(*files, parsed)) {
            values.write_logs_to_files = parsed;
        }
    }
    if (const auto directory = document->find("logging", "directory")) {
        if (!directory->empty()) {
            values.directory = std::string {*directory};
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
    if (const auto mode = document->find("logging", "log_print_mode")) {
        if (*mode == "log" || *mode == "print") {
            values.log_print_mode = std::string {*mode};
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
    return values;
}

[[nodiscard]]
bool write_values(const LoggingValues& values) {
    return cfg::write_bytes(
        ini_path(),
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

void show_settings() {
    const auto values = current_values();
    std::cout
        << "Current config/logging.ini\n"
        << "disable_all = " << (values.disable_all ? "on" : "off") << '\n'
        << "directory = " << values.directory << '\n'
        << "write_logs_to_files = "
        << (values.write_logs_to_files ? "on" : "off") << '\n'
        << "write_logs_to_console = "
        << (values.write_logs_to_console ? "on" : "off") << '\n'
        << "log_print_mode = " << values.log_print_mode << '\n'
        << "component_logging_default = "
        << (values.component_logging_default ? "on" : "off") << '\n';
}

[[nodiscard]]
int create_from_prompt() {
    const auto values = logging::prompt_values(
        std::cin,
        std::cout,
        logging::compiled_defaults()
    );
    if (!values) {
        std::cerr << "config/logging.ini was not created.\n";
        return 1;
    }
    if (!write_values(*values)) {
        std::cerr << "Failed to write " << ini_path().string() << ".\n";
        return 1;
    }
    logging_config.log_main("logging_config.exe started");
    logging_config.log_print("Wrote {}", ini_path().string());
    return 0;
}

int configuration_mode() {
    show_settings();
    while (true) {
        std::cout
            << "\nlogging_config\n"
            << "1. Modify settings\n"
            << "2. Restore defaults\n"
            << "3. Exit\n"
            << "Choice: ";
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        logging_config.log("Choice: {}", *line);
        if (*line == "1") {
            const auto values = logging::prompt_values(
                std::cin,
                std::cout,
                current_values()
            );
            if (!values) {
                return 1;
            }
            if (!write_values(*values)) {
                logging_config.log_print(
                    "Failed to write {}.",
                    ini_path().string()
                );
                return 1;
            }
            logging_config.log_print("Wrote {}", ini_path().string());
            show_settings();
        }
        else if (*line == "2") {
            if (!write_values({})) {
                logging_config.log_print(
                    "Failed to restore defaults in {}.",
                    ini_path().string()
                );
                return 1;
            }
            logging_config.log_print(
                "Restored defaults in {}",
                ini_path().string()
            );
            show_settings();
        }
        else if (*line == "3" || line->empty()) {
            return 0;
        }
        else {
            std::cout << "Enter 1, 2, or 3.\n";
        }
    }
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
        logging_config.log_main("logging_config.exe started");
        req::log_configuration_initialized(logging_config);
        return 0;
    }
    if (action == logging::Action::seed) {
        if (!write_values(logging::compiled_defaults())) {
            std::cerr << "Failed to write " << ini_path().string() << ".\n";
            return 1;
        }
        logging_config.log_main("logging_config.exe started");
        req::log_writing_defaults(logging_config);
        req::log_configuration_initialized(logging_config);
        return 0;
    }
    if (action == logging::Action::initialize) {
        req::log_configuration_missing(logging_config);
        return create_from_prompt();
    }
    if (action == logging::Action::create_missing) {
        return create_from_prompt();
    }

    logging_config.log_main("logging_config.exe started");
    req::log_config_request(logging_config, *launch);
    if (launch->seed) {
        req::log_seed_skipped(logging_config, "config/logging.ini");
        return 0;
    }
    if (disable_requested) {
        logging_config.log_print(
            "Disable skipped; config/logging.ini already exists"
        );
        return 0;
    }
    if (launch->init) {
        req::log_initialization_skipped(logging_config, "config/logging.ini");
        return 0;
    }
    return configuration_mode();
}
