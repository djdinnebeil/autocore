import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import components_editor_request;
import config_menu;

import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;
namespace menu = ac::config_menu;

namespace {

ac::Component auto_core_config {"auto_core_config"};

constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting menu_settings[] {
    {
        .key = "warn_without_winkey_mapping",
        .display_name = "Warn without Win+key mapping",
        .summary =
            "Warn when Auto Core is not in taskbar positions 1 through 10.",
        .default_value = "on",
        .choices = on_off,
    },
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging for auto_core.exe.",
        .default_value = "on",
        .choices = on_off,
    },
};

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
    if (*value == "off") {
        return false;
    }
    if (*value == "on") {
        return true;
    }
    return fallback;
}

[[nodiscard]]
bool write_settings(const bool warn, const bool logging) {
    return cfg::write_bytes(
        ini_path(),
        menu::with_header(
            menu_settings,
            defaults::ini_for_auto_core(warn, logging)
        )
    );
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    if (setting.key == "warn_without_winkey_mapping") {
        return current_warn(defaults::warn_without_winkey_mapping) ? "on" : "off";
    }
    return current_logging(ac::logging::config::component_logging_default())
        ? "on"
        : "off";
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    const bool warn = setting.key == "warn_without_winkey_mapping"
        ? value == "on"
        : current_warn(defaults::warn_without_winkey_mapping);
    const bool logging = setting.key == "logging"
        ? value == "on"
        : current_logging(ac::logging::config::component_logging_default());
    if (!write_settings(warn, logging)) {
        auto_core_config.log_print("Failed to write {}.", ini_path().string());
        return menu::ApplyResult::failed;
    }
    auto_core_config.log_print("Wrote {}", ini_path().string());
    return menu::ApplyResult::stored;
}

[[nodiscard]]
int configure_settings() {
    const auto result = menu::run_menu(
        "Auto Core Configuration",
        menu_settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
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
        << "0. Exit\n"
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
        else if (*line == "0" || line->empty()) {
            return 0;
        }
        else {
            std::cout << "Enter 0-5.\n";
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

    if (!write_settings(
            defaults::warn_without_winkey_mapping,
            defaults::auto_core_logging
        )) {
        auto_core_config.log_print("Failed to write {}.", path.string());
        return 1;
    }
    auto_core_config.log_print("Wrote {}", path.string());
    const auto offer = menu::offer_configuration(
        "Auto Core Configuration",
        menu_settings,
        current_text,
        std::cin,
        std::cout
    );
    if (offer == menu::OfferResult::failed) {
        return 1;
    }
    if (offer == menu::OfferResult::configure) {
        return configure_settings();
    }
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
