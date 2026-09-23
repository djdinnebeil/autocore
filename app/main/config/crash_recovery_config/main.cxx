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

ac::Component crash_recovery_config {"crash_recovery_config"};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "crash_recovery.ini";
}

[[nodiscard]]
std::string current_response() {
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return std::string {defaults::crash_default_response};
    }
    const auto value = document->find("dialog", "default_response");
    if (!value || (*value != "yes" && *value != "no")) {
        return std::string {defaults::crash_default_response};
    }
    return std::string {*value};
}

[[nodiscard]]
bool write_response(const std::string_view response) {
    return cfg::write_bytes(
        ini_path(),
        defaults::ini_for_crash_recovery(response)
    );
}

void show_settings() {
    std::cout
        << "Current config/crash_recovery.ini\n"
        << "  default_response = "
        << current_response()
        << '\n';
}

[[nodiscard]]
std::optional<std::string> prompt_response(const std::string_view suggestion) {
    const auto line = cfg::prompt_text(
        "default_response (yes or no)",
        suggestion
    );
    if (!line) {
        return std::nullopt;
    }
    if (*line == "yes" || *line == "no") {
        return *line;
    }
    std::cout << "Enter yes or no. Using " << suggestion << ".\n";
    return std::string {suggestion};
}

[[nodiscard]]
int first_time() {
    std::cout
        << "config/crash_recovery.ini is missing. Create it using the "
           "defaults.\n";
    const auto response = prompt_response(defaults::crash_default_response);
    if (!response) {
        return 1;
    }
    if (!write_response(*response)) {
        crash_recovery_config.log_and_print(
            "Failed to write {}.",
            ini_path().string()
        );
        return 1;
    }
    crash_recovery_config.log_and_print("Wrote {}", ini_path().string());
    return 0;
}

int configuration_mode() {
    show_settings();
    while (true) {
        std::cout
            << "\ncrash_recovery_config\n"
            << "  1. Modify settings\n"
            << "  2. Restore defaults\n"
            << "  3. Exit\n"
            << "Choice: ";
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        if (*line == "1") {
            const auto response = prompt_response(current_response());
            if (!response) {
                return 1;
            }
            if (!write_response(*response)) {
                crash_recovery_config.log_and_print(
                    "Failed to write {}.",
                    ini_path().string()
                );
                return 1;
            }
            crash_recovery_config.log_and_print("Wrote {}", ini_path().string());
            show_settings();
        }
        else if (*line == "2") {
            if (!write_response(defaults::crash_default_response)) {
                crash_recovery_config.log_and_print(
                    "Failed to restore defaults in {}.",
                    ini_path().string()
                );
                return 1;
            }
            crash_recovery_config.log_and_print(
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
    crash_recovery_config.connect_to_logger();
    crash_recovery_config.log_and_log("crash_recovery_config.exe started");

    std::error_code error;
    if (std::filesystem::exists(ini_path(), error)) {
        return configuration_mode();
    }
    if (error) {
        crash_recovery_config.log_and_print(
            "Failed to inspect {}",
            ini_path().string()
        );
        return 1;
    }
    return first_time();
}
