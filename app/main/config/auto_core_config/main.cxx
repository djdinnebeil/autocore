import std;
import auto_core.core.component;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;

import <iostream>;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;

namespace {

ac::Component auto_core_config {"auto_core_config"};

constexpr std::string_view helpers[] = {
    "main_config.exe",
    "components_config.exe",
    "keymap_config.exe",
    "keymap_editor.exe",
    "shutdown_config.exe",
    "crash_recovery_config.exe"
};

[[nodiscard]]
bool run_all_helpers() {
    for (const auto name : helpers) {
        std::cout << "Running " << name << "...\n";
        if (cfg::run_config_exe(name) != 0) {
            auto_core_config.log_and_print(
                "{} did not complete successfully.",
                name
            );
            return false;
        }
    }
    return true;
}

[[nodiscard]]
bool write_sentinel() {
    return cfg::write_bytes(
        ac::paths::config_directory() / "auto_core.ini",
        defaults::auto_core_ini
    );
}

void print_menu() {
    std::cout
        << "\nauto_core_config\n"
        << "  1. main_config.exe\n"
        << "  2. components_config.exe\n"
        << "  3. keymap_config.exe\n"
        << "  4. keymap_editor.exe\n"
        << "  5. shutdown_config.exe\n"
        << "  6. crash_recovery_config.exe\n"
        << "  7. Run all six configuration programs\n"
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
            (void)cfg::run_config_exe("main_config.exe");
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
    auto_core_config.connect_to_logger();
    auto_core_config.log_and_log("auto_core_config.exe started");

    const auto path = ac::paths::config_directory() / "auto_core.ini";
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        return configuration_mode();
    }
    if (error) {
        auto_core_config.log_and_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }

    if (!run_all_helpers()) {
        auto_core_config.log_and_print(
            "Initial configuration did not finish. "
            "config/auto_core.ini was not created."
        );
        return 1;
    }
    if (!write_sentinel()) {
        auto_core_config.log_and_print("Failed to write {}.", path.string());
        return 1;
    }
    auto_core_config.log_and_print("Wrote {}", path.string());
    return 0;
}
