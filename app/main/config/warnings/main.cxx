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

ac::Component warnings_config {"warnings_config"};

constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting settings[] {
    {
        .key = "missing_winkey_mapping",
        .display_name = "Missing Win+key mapping",
        .summary =
            "Warn when Auto Core is not in taskbar positions 1 through 10.",
        .default_value = "on",
        .choices = on_off,
    },
    {
        .key = "unmapped_key",
        .display_name = "Unmapped key",
        .summary = "Warn at load time for keys that have no command.",
        .default_value = "on",
        .choices = on_off,
    },
};

struct WarningFlags {
    bool missing_winkey_mapping {defaults::warnings_missing_winkey_mapping};
    bool unmapped_key {defaults::warnings_unmapped_key};
};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "warnings.ini";
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
WarningFlags current_flags() {
    WarningFlags flags;
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return flags;
    }
    flags.missing_winkey_mapping = enabled_value(
        document->find("warnings", "missing_winkey_mapping"),
        defaults::warnings_missing_winkey_mapping
    );
    flags.unmapped_key = enabled_value(
        document->find("warnings", "unmapped_key"),
        defaults::warnings_unmapped_key
    );
    return flags;
}

[[nodiscard]]
bool write_flags(const WarningFlags& flags) {
    return cfg::write_bytes(
        ini_path(),
        menu::with_header(
            settings,
            defaults::ini_for_warnings(
                flags.missing_winkey_mapping,
                flags.unmapped_key
            )
        )
    );
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    const auto flags = current_flags();
    const bool enabled = setting.key == "missing_winkey_mapping"
        ? flags.missing_winkey_mapping
        : flags.unmapped_key;
    return enabled ? "on" : "off";
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    WarningFlags flags = current_flags();
    const bool enabled = value == "on";
    if (setting.key == "missing_winkey_mapping") {
        flags.missing_winkey_mapping = enabled;
    }
    else {
        flags.unmapped_key = enabled;
    }
    if (!write_flags(flags)) {
        warnings_config.print("Failed to write {}.", ini_path().string());
        return menu::ApplyResult::failed;
    }
    warnings_config.log_debug("Wrote {}", ini_path().string());
    return menu::ApplyResult::stored;
}

[[nodiscard]]
int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Warnings Configuration",
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
    if (!write_flags({})) {
        warnings_config.print("Failed to write {}.", ini_path().string());
        return 1;
    }
    warnings_config.log_debug("Wrote {}", ini_path().string());
    return 0;
}

[[nodiscard]]
int initialize_missing() {
    if (write_missing_defaults() != 0) {
        return 1;
    }
    const auto result = menu::prompt_settings(
        settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout,
        "Warnings Configuration"
    );
    return result.ok ? 0 : 1;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    warnings_config.log_info("warnings_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    std::error_code error;
    const bool present = std::filesystem::exists(ini_path(), error);
    if (error) {
        warnings_config.print("Failed to inspect {}", ini_path().string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(warnings_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(warnings_config, "config/warnings.ini");
            return req::finish_configuration_initialization(
                warnings_config,
                0,
                "Warnings"
            );
        }
        req::log_writing_defaults(warnings_config);
        if (write_missing_defaults() != 0) {
            return 1;
        }
        req::log_configuration_initialized(warnings_config);
        return req::finish_configuration_initialization(
            warnings_config,
            0,
            "Warnings"
        );
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(
                warnings_config,
                "config/warnings.ini"
            );
            return req::finish_configuration_initialization(
                warnings_config,
                0,
                "Warnings"
            );
        }
        req::log_configuration_missing(warnings_config);
        req::log_writing_defaults(warnings_config);
        if (initialize_missing() != 0) {
            return 1;
        }
        req::log_configuration_initialized(warnings_config);
        return req::finish_configuration_initialization(
            warnings_config,
            0,
            "Warnings"
        );
    }

    if (!present) {
        req::log_configuration_missing(warnings_config);
        req::log_writing_defaults(warnings_config);
        if (write_missing_defaults() != 0) {
            return 1;
        }
    }
    return run_configuration_menu();
}
