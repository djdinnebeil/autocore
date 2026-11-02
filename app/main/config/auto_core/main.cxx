import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import auto_core.main.installed_version;
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
constexpr std::string_view version_value[] {defaults::auto_core_version};

const menu::Setting menu_settings[] {
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging for auto_core.exe.",
        .default_value = "on",
        .choices = on_off,
    },
    {
        .key = "check_for_updates",
        .display_name = "Check for updates",
        .summary = "Checks GitHub for a newer Auto Core version at startup.",
        .default_value = "on",
        .choices = on_off,
    },
};

struct AutoCoreFlags {
    bool logging;
    bool check_for_updates;
};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "auto_core.ini";
}

[[nodiscard]]
bool enabled_value(
    const std::optional<std::string_view> value,
    const bool fallback
) {
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
AutoCoreFlags current_flags() {
    AutoCoreFlags flags {
        .logging = ac::logging::config::component_logging_default(),
        .check_for_updates = defaults::auto_core_check_for_updates,
    };
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return flags;
    }
    flags.logging = enabled_value(
        document->find("auto_core", "logging"),
        flags.logging
    );
    flags.check_for_updates = enabled_value(
        document->find("auto_core", "check_for_updates"),
        flags.check_for_updates
    );
    return flags;
}

[[nodiscard]]
bool write_settings(const AutoCoreFlags& flags) {
    const menu::Setting header[] {
        {
            .key = "version",
            .choices = version_value,
        },
        {
            .key = "check_for_updates",
            .choices = on_off,
        },
        {
            .key = "logging",
            .choices = on_off,
        },
    };
    return cfg::write_bytes(
        ini_path(),
        menu::with_header(
            header,
            defaults::ini_for_auto_core(
                flags.logging,
                flags.check_for_updates
            )
        )
    );
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    const auto flags = current_flags();
    const bool enabled = setting.key == "check_for_updates"
        ? flags.check_for_updates
        : flags.logging;
    return enabled ? "on" : "off";
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    AutoCoreFlags flags = current_flags();
    const bool enabled = value == "on";
    if (setting.key == "check_for_updates") {
        flags.check_for_updates = enabled;
    }
    else {
        flags.logging = enabled;
    }
    if (!write_settings(flags)) {
        auto_core_config.print("Failed to write {}.", ini_path().string());
        return menu::ApplyResult::failed;
    }
    auto_core_config.log_debug("Wrote {}", ini_path().string());
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
        auto_core_config.print(
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
        << "2. Warnings\n"
        << "3. Component policy\n"
        << "4. Shutdown settings\n"
        << "5. Crash recovery settings\n"
        << "6. Startup settings\n"
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
            launch_owner("warnings_config.exe");
        }
        else if (*line == "3") {
            launch_owner("components_policy_config.exe");
        }
        else if (*line == "4") {
            launch_owner("shutdown_config.exe");
        }
        else if (*line == "5") {
            launch_owner("crash_recovery_config.exe");
        }
        else if (*line == "6") {
            launch_owner("startup_config.exe");
        }
        else if (*line == "0" || line->empty()) {
            return 0;
        }
        else {
            std::cout << "Enter 0-6.\n";
        }
    }
}

[[nodiscard]]
int create_auto_core_ini(const bool prompt, const bool walk_settings = false) {
    const auto path = ini_path();
    if (!prompt) {
        if (!write_settings({
                .logging = defaults::auto_core_logging,
                .check_for_updates = defaults::auto_core_check_for_updates,
            })) {
            auto_core_config.print("Failed to write {}.", path.string());
            return 1;
        }
        auto_core_config.log_debug("Wrote {}", path.string());
        return 0;
    }

    if (!write_settings({
            .logging = defaults::auto_core_logging,
            .check_for_updates = defaults::auto_core_check_for_updates,
        })) {
        auto_core_config.print("Failed to write {}.", path.string());
        return 1;
    }
    auto_core_config.log_debug("Wrote {}", path.string());
    if (walk_settings) {
        const auto result = menu::prompt_settings(
            menu_settings,
            current_text,
            apply_setting,
            std::cin,
            std::cout,
            "Runtime Configuration"
        );
        return result.ok ? 0 : 1;
    }
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
    auto_core_config.log_info("auto_core_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ini_path();
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        auto_core_config.print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(auto_core_config, *launch);
    if (present && !ac::main::synchronize_installed_version()) {
        auto_core_config.print("Failed to write {}.", path.string());
        return 1;
    }
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(auto_core_config, "config/auto_core.ini");
            return req::finish_configuration_initialization(
                auto_core_config,
                0,
                "Runtime"
            );
        }
        req::log_writing_defaults(auto_core_config);
        if (create_auto_core_ini(false) != 0) {
            return 1;
        }
        req::log_configuration_initialized(auto_core_config);
        return req::finish_configuration_initialization(
            auto_core_config,
            0,
            "Runtime"
        );
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(
                auto_core_config,
                "config/auto_core.ini"
            );
            return req::finish_configuration_initialization(
                auto_core_config,
                0,
                "Runtime"
            );
        }
        req::log_configuration_missing(auto_core_config);
        return req::finish_configuration_initialization(
            auto_core_config,
            create_auto_core_ini(true, true),
            "Runtime"
        );
    }

    if (present) {
        return configuration_mode();
    }
    return create_auto_core_ini(true);
}
