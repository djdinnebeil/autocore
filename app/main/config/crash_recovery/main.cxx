import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import components_editor_request;

import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;

namespace {

ac::Component crash_recovery_config {"crash_recovery_config"};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "crash_recovery.ini";
}

struct Settings {
    std::string response {defaults::crash_default_response};
    bool diagnostics {defaults::crash_diagnostics};
};

[[nodiscard]]
Settings current_settings() {
    Settings settings;
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return settings;
    }
    if (const auto value = document->find(
            "crash_recovery", "default_response"
        ); value && (*value == "yes" || *value == "no")) {
        settings.response = std::string {*value};
    }
    if (const auto value = document->find(
            "crash_recovery", "crash_diagnostics"
        ); value && (*value == "on" || *value == "off")) {
        settings.diagnostics = *value == "on";
    }
    return settings;
}

[[nodiscard]]
bool write_settings(const Settings& settings) {
    return cfg::write_bytes(
        ini_path(),
        defaults::ini_for_crash_recovery(
            settings.response,
            settings.diagnostics
        )
    );
}

void show_settings() {
    const auto settings = current_settings();
    std::cout
        << "Current config/crash_recovery.ini\n"
        << "  default_response = "
        << settings.response
        << "\n  crash_diagnostics = "
        << (settings.diagnostics ? "on" : "off")
        << '\n';
}

[[nodiscard]]
std::optional<std::string> prompt_response(const std::string_view suggestion) {
    const auto line = cfg::prompt_text(
        "default_response (yes or no)",
        suggestion
    );
    if (!line) {
        return std::nullopt;
    }
    if (*line == "yes" || *line == "no") {
        return *line;
    }
    std::cout << "Enter yes or no. Using " << suggestion << ".\n";
    return std::string {suggestion};
}

[[nodiscard]]
std::optional<bool> prompt_diagnostics(const bool suggestion) {
    const auto line = cfg::prompt_text(
        "crash_diagnostics (on or off)",
        suggestion ? "on" : "off"
    );
    if (!line) {
        return std::nullopt;
    }
    if (*line == "on" || *line == "off") {
        return *line == "on";
    }
    std::cout
        << "Enter on or off. Using "
        << (suggestion ? "on" : "off")
        << ".\n";
    return suggestion;
}

[[nodiscard]]
int first_time() {
    std::cout
        << "config/crash_recovery.ini is missing. Create it using the "
           "defaults.\n";
    Settings settings;
    const auto response = prompt_response(settings.response);
    if (!response) {
        return 1;
    }
    settings.response = *response;
    const auto diagnostics = prompt_diagnostics(settings.diagnostics);
    if (!diagnostics) {
        return 1;
    }
    settings.diagnostics = *diagnostics;
    if (!write_settings(settings)) {
        crash_recovery_config.log_print(
            "Failed to write {}.",
            ini_path().string()
        );
        return 1;
    }
    crash_recovery_config.log_print("Wrote {}", ini_path().string());
    return 0;
}

int configuration_mode() {
    show_settings();
    while (true) {
        std::cout
            << "\ncrash_recovery_config\n"
            << "  1. Modify settings\n"
            << "  2. Restore defaults\n"
            << "  3. Exit\n"
            << "Choice: ";
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        if (*line == "1") {
            auto settings = current_settings();
            const auto response = prompt_response(settings.response);
            if (!response) {
                return 1;
            }
            settings.response = *response;
            const auto diagnostics = prompt_diagnostics(settings.diagnostics);
            if (!diagnostics) {
                return 1;
            }
            settings.diagnostics = *diagnostics;
            if (!write_settings(settings)) {
                crash_recovery_config.log_print(
                    "Failed to write {}.",
                    ini_path().string()
                );
                return 1;
            }
            crash_recovery_config.log_print("Wrote {}", ini_path().string());
            show_settings();
        }
        else if (*line == "2") {
            if (!write_settings(Settings {})) {
                crash_recovery_config.log_print(
                    "Failed to restore defaults in {}.",
                    ini_path().string()
                );
                return 1;
            }
            crash_recovery_config.log_print(
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
    crash_recovery_config.log_main("crash_recovery_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    std::error_code error;
    const bool present = std::filesystem::exists(ini_path(), error);
    if (error) {
        crash_recovery_config.log_print(
            "Failed to inspect {}",
            ini_path().string()
        );
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(crash_recovery_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(
                crash_recovery_config,
                "config/crash_recovery.ini"
            );
            return 0;
        }
        req::log_writing_defaults(crash_recovery_config);
        if (!write_settings(Settings {})) {
            crash_recovery_config.log_print(
                "Failed to write {}.",
                ini_path().string()
            );
            return 1;
        }
        req::log_configuration_initialized(crash_recovery_config);
        return 0;
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(
                crash_recovery_config,
                "config/crash_recovery.ini"
            );
            return 0;
        }
        req::log_configuration_missing(crash_recovery_config);
        return first_time();
    }

    if (present) {
        return configuration_mode();
    }
    return first_time();
}
