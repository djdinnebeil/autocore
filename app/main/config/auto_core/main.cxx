import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;

import <iostream>;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;

namespace {

ac::Component auto_core_config {"auto_core_config"};

constexpr std::string_view helpers[] = {
    "components_config.exe",
    "keymap_config.exe",
    "keymap_editor.exe",
    "shutdown_config.exe",
    "crash_recovery_config.exe"
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
    return *value != "false";
}

[[nodiscard]]
bool write_warn(const bool warn) {
    return cfg::write_bytes(ini_path(), defaults::ini_for_auto_core(warn));
}

void show_settings() {
    const bool warn = current_warn(defaults::warn_without_winkey_mapping);
    std::cout
        << "Current config/auto_core.ini\n"
        << "  warn_without_winkey_mapping = "
        << (warn ? "true" : "false")
        << '\n';
}

[[nodiscard]]
std::optional<bool> prompt_warn(const bool suggestion) {
    return cfg::prompt_bool("warn_without_winkey_mapping", suggestion);
}

[[nodiscard]]
bool run_all_helpers() {
    for (const auto name : helpers) {
        std::cout << "Running " << name << "...\n";
        if (cfg::run_config_exe(name) != 0) {
            auto_core_config.log_print(
                "{} did not complete successfully.",
                name
            );
            return false;
        }
    }
    return true;
}

[[nodiscard]]
int configure_warning() {
    show_settings();
    while (true) {
        std::cout
            << "\nauto_core warning\n"
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
            if (!write_warn(*warn)) {
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
            if (!write_warn(defaults::warn_without_winkey_mapping)) {
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

void print_menu() {
    std::cout
        << "\nauto_core_config\n"
        << "  1. Auto Core warning setting\n"
        << "  2. components_config.exe\n"
        << "  3. keymap_config.exe\n"
        << "  4. keymap_editor.exe\n"
        << "  5. shutdown_config.exe\n"
        << "  6. crash_recovery_config.exe\n"
        << "  7. Run all five configuration programs\n"
        << "  8. Exit\n"
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
            if (configure_warning() != 0) {
                return 1;
            }
        }
        else if (*line == "2") {
            (void)cfg::run_config_exe("components_config.exe");
        }
        else if (*line == "3") {
            (void)cfg::run_config_exe("keymap_config.exe");
        }
        else if (*line == "4") {
            (void)cfg::run_config_exe("keymap_editor.exe");
        }
        else if (*line == "5") {
            (void)cfg::run_config_exe("shutdown_config.exe");
        }
        else if (*line == "6") {
            (void)cfg::run_config_exe("crash_recovery_config.exe");
        }
        else if (*line == "7") {
            (void)run_all_helpers();
        }
        else if (*line == "8" || line->empty()) {
            return 0;
        }
        else {
            std::cout << "Enter 1-8.\n";
        }
    }
}

} // namespace

int main() {
    auto_core_config.log_main("auto_core_config.exe started");

    const auto path = ini_path();
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        return configuration_mode();
    }
    if (error) {
        auto_core_config.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }

    if (!run_all_helpers()) {
        auto_core_config.log_print(
            "Initial configuration did not finish. "
            "config/auto_core.ini was not created."
        );
        return 1;
    }

    std::cout
        << "config/auto_core.ini is missing. Create it using the defaults.\n";
    const auto warn = prompt_warn(defaults::warn_without_winkey_mapping);
    if (!warn) {
        auto_core_config.log_print(
            "Initial configuration did not finish. "
            "config/auto_core.ini was not created."
        );
        return 1;
    }
    if (!write_warn(*warn)) {
        auto_core_config.log_print("Failed to write {}.", path.string());
        return 1;
    }
    auto_core_config.log_print("Wrote {}", path.string());
    return 0;
}
