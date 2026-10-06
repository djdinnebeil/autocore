import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.logging.config;
import auto_core.core.paths;
import dash_defaults;
import components_editor_request;

import <fstream>;
import <iostream>;
import auto_core.core.shell;

namespace {

    std::string trim(std::string_view value) {
        while (!value.empty() &&
               (value.front() == ' ' || value.front() == '\t')) {
            value.remove_prefix(1);
        }
        while (!value.empty() &&
               (value.back() == ' ' || value.back() == '\t' ||
                value.back() == '\r')) {
            value.remove_suffix(1);
        }
        return std::string {value};
    }

    bool logging_in_file(const std::filesystem::path& path) {
        const auto document = ac::ini::read(path);
        if (!document) {
            return false;
        }
        const auto value = document->find("dash", "logging");
        if (!value) {
            return false;
        }
        if (*value == "on") {
            return true;
        }
        return false;
    }

    std::optional<bool> prompt_logging(const bool current) {
        while (true) {
            std::cout << "Enable logging? ["
                      << (current ? "Y/n" : "y/N")
                      << "]: ";
            std::string input;
            if (!std::getline(std::cin, input)) {
                return std::nullopt;
            }
            const auto value = trim(input);
            if (value.empty()) {
                return current;
            }
            if (value == "y" || value == "Y") {
                return true;
            }
            if (value == "n" || value == "N") {
                return false;
            }
            std::cout << "Enter Y or n.\n";
        }
    }

    bool write_dash_ini(
        ac::Component& log,
        const std::filesystem::path& path,
        const bool logging
    ) {
        std::error_code create_error;
        std::filesystem::create_directories(path.parent_path(), create_error);
        if (create_error) {
            log.log_print(
                "Failed to create config directory: {}",
                create_error.message()
            );
            return false;
        }
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            log.log_print("Failed to create {}", path.string());
            return false;
        }
        const auto contents = dash::defaults::ini_for(logging);
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.close();
        if (!output) {
            log.log_print("Failed to write {}", path.string());
            return false;
        }
        return true;
    }

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    ac::Component dash_config {
        "dash_config",
        ac::logging::config::LoggingScope {
            "dash",
            ac::logging::config::LoggingFallback::off
        }
    };
    dash_config.log_main("dash_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ac::paths::config_directory() / "dash.ini";
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        dash_config.log_print("Failed to inspect {}", path.string());
        return 1;
    }
    namespace req = ac::config::components_request;
    req::log_config_request(dash_config, *launch);
    if (exists) {
        if (launch->seed) {
            req::log_seed_skipped(dash_config, "config/dash.ini");
            return 0;
        }
        if (launch->init) {
            req::log_initialization_skipped(dash_config, "config/dash.ini");
            return 0;
        }
        const bool current = logging_in_file(path);
        std::cout << "logging = " << (current ? "on" : "off") << '\n'
                  << "The vault is %LOCALAPPDATA%\\Auto Core\\dash.vault.\n";
        const auto logging = prompt_logging(current);
        if (!logging) {
            return 1;
        }
        if (!write_dash_ini(dash_config, path, *logging)) {
            return 1;
        }
        dash_config.log_print("Wrote {}", path.string());
        return 0;
    }
    if (launch->seed) {
        req::log_writing_defaults(dash_config);
        if (!write_dash_ini(dash_config, path, false)) {
            return 1;
        }
        dash_config.log_print(
            "Wrote default {} (vault path is not configured here).",
            path.string()
        );
        req::log_configuration_initialized(dash_config);
        return 0;
    }
    if (launch->init) {
        req::log_configuration_missing(dash_config);
    }
    const auto logging = prompt_logging(false);
    if (!logging) {
        return 1;
    }
    if (!write_dash_ini(dash_config, path, *logging)) {
        return 1;
    }
    dash_config.log_print(
        "Wrote {} (vault path is not configured here).",
        path.string()
    );
    return 0;
}
