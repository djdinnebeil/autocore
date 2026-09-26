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

ac::Component shutdown_config {"shutdown_config"};

struct ShutdownValues {
    std::string prompt {defaults::shutdown_prompt};
    unsigned timeout_ms {defaults::shutdown_timeout_ms};
};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "shutdown.ini";
}

[[nodiscard]]
ShutdownValues current_values() {
    ShutdownValues values;
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return values;
    }
    if (const auto prompt = document->find("shutdown", "delayed_shutdown_prompt");
        prompt && (*prompt == "popup" || *prompt == "console")) {
        values.prompt = std::string {*prompt};
    }
    if (const auto timeout = document->find("shutdown", "shutdown_timeout_ms")) {
        unsigned milliseconds = 0;
        const auto parsed = std::from_chars(
            timeout->data(),
            timeout->data() + timeout->size(),
            milliseconds
        );
        if (parsed.ec == std::errc {} &&
            parsed.ptr == timeout->data() + timeout->size()) {
            values.timeout_ms = milliseconds;
        }
    }
    return values;
}

[[nodiscard]]
bool write_values(const ShutdownValues& values) {
    return cfg::write_bytes(
        ini_path(),
        defaults::ini_for_shutdown(values.prompt, values.timeout_ms)
    );
}

void show_settings() {
    const auto values = current_values();
    std::cout
        << "Current config/shutdown.ini\n"
        << "  delayed_shutdown_prompt = "
        << values.prompt
        << "\n  shutdown_timeout_ms = "
        << values.timeout_ms
        << '\n';
}

[[nodiscard]]
std::optional<ShutdownValues> prompt_values(const ShutdownValues& suggestion) {
    ShutdownValues values = suggestion;
    const auto prompt = cfg::prompt_text(
        "delayed_shutdown_prompt (popup or console)",
        suggestion.prompt
    );
    if (!prompt) {
        return std::nullopt;
    }
    if (*prompt == "popup" || *prompt == "console") {
        values.prompt = *prompt;
    }
    else {
        std::cout << "Enter popup or console. Using " << suggestion.prompt
                  << ".\n";
        values.prompt = suggestion.prompt;
    }

    const auto timeout = cfg::prompt_text(
        "shutdown_timeout_ms",
        std::to_string(suggestion.timeout_ms)
    );
    if (!timeout) {
        return std::nullopt;
    }
    unsigned milliseconds = 0;
    const auto parsed = std::from_chars(
        timeout->data(),
        timeout->data() + timeout->size(),
        milliseconds
    );
    if (parsed.ec == std::errc {} &&
        parsed.ptr == timeout->data() + timeout->size()) {
        values.timeout_ms = milliseconds;
    }
    else {
        std::cout << "Enter a non-negative integer. Using "
                  << suggestion.timeout_ms << ".\n";
        values.timeout_ms = suggestion.timeout_ms;
    }
    return values;
}

[[nodiscard]]
int first_time() {
    std::cout
        << "config/shutdown.ini is missing. Create it using the defaults.\n";
    const auto values = prompt_values({});
    if (!values) {
        return 1;
    }
    if (!write_values(*values)) {
        shutdown_config.log_print(
            "Failed to write {}.",
            ini_path().string()
        );
        return 1;
    }
    shutdown_config.log_print("Wrote {}", ini_path().string());
    return 0;
}

int configuration_mode() {
    show_settings();
    while (true) {
        std::cout
            << "\nshutdown_config\n"
            << "  1. Modify settings\n"
            << "  2. Restore defaults\n"
            << "  3. Exit\n"
            << "Choice: ";
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        if (*line == "1") {
            const auto values = prompt_values(current_values());
            if (!values) {
                return 1;
            }
            if (!write_values(*values)) {
                shutdown_config.log_print(
                    "Failed to write {}.",
                    ini_path().string()
                );
                return 1;
            }
            shutdown_config.log_print("Wrote {}", ini_path().string());
            show_settings();
        }
        else if (*line == "2") {
            if (!write_values({})) {
                shutdown_config.log_print(
                    "Failed to restore defaults in {}.",
                    ini_path().string()
                );
                return 1;
            }
            shutdown_config.log_print(
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
    shutdown_config.log_main("shutdown_config.exe started");

    std::error_code error;
    if (std::filesystem::exists(ini_path(), error)) {
        return configuration_mode();
    }
    if (error) {
        shutdown_config.log_print(
            "Failed to inspect {}",
            ini_path().string()
        );
        return 1;
    }
    return first_time();
}
