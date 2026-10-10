import std;
import auto_core.core.component;
import auto_core.core.ini;
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

ac::Component crash_recovery_config {"crash_recovery_config"};

constexpr std::string_view yes_no[] {"yes", "no"};
constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting settings[] {
    {
        .key = "default_response",
        .display_name = "Default response",
        .summary = "Answer Main uses for the previous-crash prompt.",
        .default_value = defaults::crash_default_response,
        .choices = yes_no,
    },
    {
        .key = "crash_diagnostics",
        .display_name = "Crash diagnostics",
        .summary = "Write shared crash reports for every Auto Core executable.",
        .default_value = "on",
        .choices = on_off,
    },
};

struct Settings {
    std::string response {defaults::crash_default_response};
    bool diagnostics {defaults::crash_diagnostics};
};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "crash_recovery.ini";
}

[[nodiscard]]
Settings current_settings() {
    Settings loaded;
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return loaded;
    }
    if (const auto value = document->find(
            "crash_recovery", "default_response"
        ); value && (*value == "yes" || *value == "no")) {
        loaded.response = std::string {*value};
    }
    if (const auto value = document->find(
            "crash_recovery", "crash_diagnostics"
        ); value && (*value == "on" || *value == "off")) {
        loaded.diagnostics = *value == "on";
    }
    return loaded;
}

[[nodiscard]]
bool write_settings(const Settings& loaded) {
    return cfg::write_bytes(
        ini_path(),
        menu::with_header(
            settings,
            defaults::ini_for_crash_recovery(
                loaded.response,
                loaded.diagnostics
            )
        )
    );
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    const auto loaded = current_settings();
    if (setting.key == "default_response") {
        return loaded.response;
    }
    return loaded.diagnostics ? "on" : "off";
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    auto loaded = current_settings();
    if (setting.key == "default_response") {
        loaded.response = std::string {value};
    }
    else {
        loaded.diagnostics = value == "on";
    }
    if (!write_settings(loaded)) {
        crash_recovery_config.log_print(
            "Failed to write {}.",
            ini_path().string()
        );
        return menu::ApplyResult::failed;
    }
    crash_recovery_config.log_print("Wrote {}", ini_path().string());
    return menu::ApplyResult::stored;
}

[[nodiscard]]
int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Crash Recovery Configuration",
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
    if (!write_settings(Settings {})) {
        crash_recovery_config.log_print(
            "Failed to write {}.",
            ini_path().string()
        );
        return 1;
    }
    crash_recovery_config.log_print("Wrote {}", ini_path().string());
    return 0;
}

[[nodiscard]]
int initialize_missing() {
    if (write_missing_defaults() != 0) {
        return 1;
    }
    const auto offer = menu::offer_configuration(
        "Crash Recovery Configuration",
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
        if (write_missing_defaults() != 0) {
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
        req::log_writing_defaults(crash_recovery_config);
        if (initialize_missing() != 0) {
            return 1;
        }
        req::log_configuration_initialized(crash_recovery_config);
        return 0;
    }

    if (!present) {
        req::log_configuration_missing(crash_recovery_config);
        req::log_writing_defaults(crash_recovery_config);
        if (write_missing_defaults() != 0) {
            return 1;
        }
    }
    return run_configuration_menu();
}
