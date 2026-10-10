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

ac::Component keymap_config {"keymap_config"};

constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting settings[] {
    {
        .key = "silence_nonset_warning",
        .display_name = "Silence unset-key warning",
        .summary = "Skip the load-time message for keys that have no command.",
        .default_value = "off",
        .choices = on_off,
    },
};

struct KeymapFlags {
    bool silence_nonset_warning {defaults::keymap_silence_nonset_warning};
};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::keymap_settings_file();
}

[[nodiscard]]
KeymapFlags current_flags() {
    KeymapFlags flags;
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return flags;
    }
    flags.silence_nonset_warning =
        document->find("keymap", "silence_nonset_warning") == "on";
    return flags;
}

[[nodiscard]]
bool write_flags(const KeymapFlags& flags) {
    return cfg::write_bytes(
        ini_path(),
        menu::with_header(
            settings,
            defaults::ini_for_keymap(flags.silence_nonset_warning)
        )
    );
}

[[nodiscard]]
std::string current_text(const menu::Setting&) {
    return current_flags().silence_nonset_warning ? "on" : "off";
}

[[nodiscard]]
menu::ApplyResult apply_setting(const menu::Setting&, const std::string_view value) {
    KeymapFlags flags;
    flags.silence_nonset_warning = value == "on";
    if (!write_flags(flags)) {
        keymap_config.log_print("Failed to write {}.", ini_path().string());
        return menu::ApplyResult::failed;
    }
    keymap_config.log_print("Wrote {}", ini_path().string());
    return menu::ApplyResult::stored;
}

[[nodiscard]]
int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Keymap Configuration",
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
        keymap_config.log_print("Failed to write {}.", ini_path().string());
        return 1;
    }
    keymap_config.log_print("Wrote {}", ini_path().string());
    return 0;
}

[[nodiscard]]
int initialize_missing() {
    if (write_missing_defaults() != 0) {
        return 1;
    }
    const auto offer = menu::offer_configuration(
        "Keymap Configuration",
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
    keymap_config.log_main("keymap_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    std::error_code error;
    const bool present = std::filesystem::exists(ini_path(), error);
    if (error) {
        keymap_config.log_print("Failed to inspect {}", ini_path().string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(keymap_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(keymap_config, "config/keymap.ini");
            return 0;
        }
        req::log_writing_defaults(keymap_config);
        if (write_missing_defaults() != 0) {
            return 1;
        }
        req::log_configuration_initialized(keymap_config);
        return 0;
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(keymap_config, "config/keymap.ini");
            return 0;
        }
        req::log_configuration_missing(keymap_config);
        req::log_writing_defaults(keymap_config);
        if (initialize_missing() != 0) {
            return 1;
        }
        req::log_configuration_initialized(keymap_config);
        return 0;
    }

    if (!present) {
        req::log_configuration_missing(keymap_config);
        req::log_writing_defaults(keymap_config);
        if (write_missing_defaults() != 0) {
            return 1;
        }
    }
    return run_configuration_menu();
}
