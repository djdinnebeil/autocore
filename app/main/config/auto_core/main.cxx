import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import components_editor_request;

import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;

namespace {

ac::Component auto_core_config {"auto_core_config"};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "auto_core.ini";
}

[[nodiscard]]
bool current_warn(const bool fallback) {
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return fallback;
    }
    const auto value = document->find("auto_core", "warn_without_winkey_mapping");
    if (!value) {
        return fallback;
    }
    return *value != "off";
}

[[nodiscard]]
bool current_logging(const bool fallback) {
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return fallback;
    }
    const auto value = document->find("auto_core", "logging");
    if (!value) {
        return fallback;
    }
    if (*value == "off" || *value == "false") {
        return false;
    }
    if (*value == "on" || *value == "true") {
        return true;
    }
    return fallback;
}

[[nodiscard]]
bool write_settings(const bool warn, const bool logging) {
    return cfg::write_bytes(
        ini_path(),
        defaults::ini_for_auto_core(warn, logging)
    );
}

void show_settings() {
    const bool warn = current_warn(defaults::warn_without_winkey_mapping);
    const bool logging = current_logging(
        ac::logging::config::component_logging_default()
    );
    std::cout
        << "Current config/auto_core.ini\n"
        << "  warn_without_winkey_mapping = "
        << (warn ? "on" : "off")
        << "\n"
        << "  logging = "
        << (logging ? "on" : "off")
        << '\n';
}

[[nodiscard]]
std::optional<bool> prompt_warn(const bool suggestion) {
    return cfg::prompt_on_off("warn_without_winkey_mapping", suggestion);
}

[[nodiscard]]
std::optional<bool> prompt_logging(const bool suggestion) {
    return cfg::prompt_on_off("logging", suggestion);
}

[[nodiscard]]
int configure_settings() {
    show_settings();
    while (true) {
        std::cout
            << "\nauto_core settings\n"
            << "  1. Modify settings\n"
            << "  2. Restore defaults\n"
            << "  3. Back\n"
            << "Choice: ";
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        if (*line == "1") {
            const auto warn = prompt_warn(
                current_warn(defaults::warn_without_winkey_mapping)
            );
            if (!warn) {
                return 1;
            }
            const auto logging = prompt_logging(
                current_logging(
                    ac::logging::config::component_logging_default()
                )
            );
            if (!logging) {
                return 1;
            }
            if (!write_settings(*warn, *logging)) {
                auto_core_config.log_print(
                    "Failed to write {}.",
                    ini_path().string()
                );
                return 1;
            }
            auto_core_config.log_print("Wrote {}", ini_path().string());
            show_settings();
        }
        else if (*line == "2") {
            if (!write_settings(
                    defaults::warn_without_winkey_mapping,
                    defaults::auto_core_logging
                )) {
                auto_core_config.log_print(
                    "Failed to restore defaults in {}.",
                    ini_path().string()
                );
                return 1;
            }
            auto_core_config.log_print(
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

void launch_owner(const std::string_view name) {
    if (cfg::run_config_exe(name) != 0) {
        auto_core_config.log_print(
            "{} did not complete successfully.",
            name
        );
    }
}

void print_menu() {
    std::cout
        << "\nAuto Core Configuration\n"
        << "\n"
        << "1. Auto Core settings\n"
        << "2. Keymap settings\n"
        << "3. Components settings\n"
        << "4. Shutdown settings\n"
        << "5. Crash recovery settings\n"
        << "6. Exit\n"
        << "Choice: ";
}

int configuration_mode() {
    while (true) {
        print_menu();
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        if (*line == "1") {
            if (configure_settings() != 0) {
                return 1;
            }
        }
        else if (*line == "2") {
            launch_owner("keymap_config.exe");
        }
        else if (*line == "3") {
            launch_owner("components_config.exe");
        }
        else if (*line == "4") {
            launch_owner("shutdown_config.exe");
        }
        else if (*line == "5") {
            launch_owner("crash_recovery_config.exe");
        }
        else if (*line == "6" || line->empty()) {
            return 0;
        }
        else {
            std::cout << "Enter 1-6.\n";
        }
    }
}

[[nodiscard]]
int create_auto_core_ini(const bool prompt) {
    const auto path = ini_path();
    if (!prompt) {
        if (!write_settings(
                defaults::warn_without_winkey_mapping,
                defaults::auto_core_logging
            )) {
            auto_core_config.log_print("Failed to write {}.", path.string());
            return 1;
        }
        auto_core_config.log_print("Wrote {}", path.string());
        return 0;
    }

    std::cout
        << "config/auto_core.ini is missing. Create it using the defaults.\n"
        << "auto_core_init.exe initializes the other configuration files.\n";
    const auto warn = prompt_warn(defaults::warn_without_winkey_mapping);
    if (!warn) {
        auto_core_config.log_print(
            "Initial configuration did not finish. "
            "config/auto_core.ini was not created."
        );
        return 1;
    }
    const auto logging = prompt_logging(defaults::auto_core_logging);
    if (!logging) {
        auto_core_config.log_print(
            "Initial configuration did not finish. "
            "config/auto_core.ini was not created."
        );
        return 1;
    }
    if (!write_settings(*warn, *logging)) {
        auto_core_config.log_print("Failed to write {}.", path.string());
        return 1;
    }
    auto_core_config.log_print("Wrote {}", path.string());
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    auto_core_config.log_main("auto_core_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ini_path();
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        auto_core_config.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(auto_core_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(auto_core_config, "config/auto_core.ini");
            return 0;
        }
        req::log_writing_defaults(auto_core_config);
        if (create_auto_core_ini(false) != 0) {
            return 1;
        }
        req::log_configuration_initialized(auto_core_config);
        return 0;
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(
                auto_core_config,
                "config/auto_core.ini"
            );
            return 0;
        }
        req::log_configuration_missing(auto_core_config);
        return create_auto_core_ini(true);
    }

    if (present) {
        return configuration_mode();
    }
    return create_auto_core_ini(true);
}
