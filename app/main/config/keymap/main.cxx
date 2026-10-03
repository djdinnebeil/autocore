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

ac::Component keymap_config {"keymap_config"};

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
        defaults::ini_for_keymap(flags.silence_nonset_warning)
    );
}

void show_settings() {
    const auto flags = current_flags();
    std::cout
        << "Current config/keymap.ini\n"
        << "  silence_nonset_warning = "
        << (flags.silence_nonset_warning ? "on" : "off")
        << '\n';
}

[[nodiscard]]
std::optional<KeymapFlags> prompt_flags(const KeymapFlags& suggestion) {
    KeymapFlags flags = suggestion;
    const auto silence = cfg::prompt_on_off(
        "silence_nonset_warning",
        suggestion.silence_nonset_warning
    );
    if (!silence) {
        return std::nullopt;
    }
    flags.silence_nonset_warning = *silence;
    return flags;
}

[[nodiscard]]
int first_time() {
    std::cout << "config/keymap.ini is missing. Create it using the defaults.\n";
    const auto flags = prompt_flags({});
    if (!flags) {
        return 1;
    }
    if (!write_flags(*flags)) {
        keymap_config.log_print("Failed to write {}.", ini_path().string());
        return 1;
    }
    keymap_config.log_print("Wrote {}", ini_path().string());
    return 0;
}

int configuration_mode() {
    show_settings();
    while (true) {
        std::cout
            << "\nkeymap_config\n"
            << "  1. Modify settings\n"
            << "  2. Restore defaults\n"
            << "  3. Exit\n"
            << "Choice: ";
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        if (*line == "1") {
            const auto flags = prompt_flags(current_flags());
            if (!flags) {
                return 1;
            }
            if (!write_flags(*flags)) {
                keymap_config.log_print(
                    "Failed to write {}.",
                    ini_path().string()
                );
                return 1;
            }
            keymap_config.log_print("Wrote {}", ini_path().string());
            show_settings();
        }
        else if (*line == "2") {
            if (!write_flags({})) {
                keymap_config.log_print(
                    "Failed to restore defaults in {}.",
                    ini_path().string()
                );
                return 1;
            }
            keymap_config.log_print(
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
        if (!write_flags({})) {
            keymap_config.log_print("Failed to write {}.", ini_path().string());
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
        return first_time();
    }

    if (present) {
        return configuration_mode();
    }
    return first_time();
}
