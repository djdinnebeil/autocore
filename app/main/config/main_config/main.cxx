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

ac::Component main_config {"main_config"};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "main.ini";
}

[[nodiscard]]
bool current_warn(const bool fallback) {
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return fallback;
    }
    const auto value = document->find("main", "warn_without_winkey_mapping");
    if (!value) {
        return fallback;
    }
    return *value != "false";
}

[[nodiscard]]
bool write_warn(const bool warn) {
    return cfg::write_bytes(ini_path(), defaults::ini_for_main(warn));
}

void show_settings() {
    const bool warn = current_warn(defaults::warn_without_winkey_mapping);
    std::cout
        << "Current config/main.ini\n"
        << "  warn_without_winkey_mapping = "
        << (warn ? "true" : "false")
        << '\n';
}

[[nodiscard]]
std::optional<bool> prompt_warn(const bool suggestion) {
    return cfg::prompt_bool("warn_without_winkey_mapping", suggestion);
}

[[nodiscard]]
int first_time() {
    std::cout << "config/main.ini is missing. Create it using the defaults.\n";
    const auto warn = prompt_warn(defaults::warn_without_winkey_mapping);
    if (!warn) {
        return 1;
    }
    if (!write_warn(*warn)) {
        main_config.log_and_print("Failed to write {}.", ini_path().string());
        return 1;
    }
    main_config.log_and_print("Wrote {}", ini_path().string());
    return 0;
}

int configuration_mode() {
    show_settings();
    while (true) {
        std::cout
            << "\nmain_config\n"
            << "  1. Modify settings\n"
            << "  2. Restore defaults\n"
            << "  3. Exit\n"
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
                main_config.log_and_print(
                    "Failed to write {}.",
                    ini_path().string()
                );
                return 1;
            }
            main_config.log_and_print("Wrote {}", ini_path().string());
            show_settings();
        }
        else if (*line == "2") {
            if (!write_warn(defaults::warn_without_winkey_mapping)) {
                main_config.log_and_print(
                    "Failed to restore defaults in {}.",
                    ini_path().string()
                );
                return 1;
            }
            main_config.log_and_print(
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

int main() {
    main_config.connect_to_logger();
    main_config.log_and_log("main_config.exe started");

    std::error_code error;
    if (std::filesystem::exists(ini_path(), error)) {
        return configuration_mode();
    }
    if (error) {
        main_config.log_and_print("Failed to inspect {}", ini_path().string());
        return 1;
    }
    return first_time();
}
