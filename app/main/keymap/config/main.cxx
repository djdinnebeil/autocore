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

ac::Component keymap_config {"keymap_config"};

struct KeymapFlags {
    bool trace_enabled {defaults::keymap_trace_enabled};
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
    flags.trace_enabled =
        document->find("keymap", "trace_enabled") == "true";
    flags.silence_nonset_warning =
        document->find("keymap", "silence_nonset_warning") == "true";
    return flags;
}

[[nodiscard]]
bool write_flags(const KeymapFlags& flags) {
    return cfg::write_bytes(
        ini_path(),
        defaults::ini_for_keymap(
            flags.trace_enabled,
            flags.silence_nonset_warning
        )
    );
}

void show_settings() {
    const auto flags = current_flags();
    std::cout
        << "Current config/keymap.ini\n"
        << "  trace_enabled = "
        << (flags.trace_enabled ? "true" : "false")
        << "\n  silence_nonset_warning = "
        << (flags.silence_nonset_warning ? "true" : "false")
        << '\n';
}

[[nodiscard]]
std::optional<KeymapFlags> prompt_flags(const KeymapFlags& suggestion) {
    KeymapFlags flags = suggestion;
    const auto trace = cfg::prompt_bool("trace_enabled", suggestion.trace_enabled);
    if (!trace) {
        return std::nullopt;
    }
    const auto silence = cfg::prompt_bool(
        "silence_nonset_warning",
        suggestion.silence_nonset_warning
    );
    if (!silence) {
        return std::nullopt;
    }
    flags.trace_enabled = *trace;
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
        keymap_config.log_and_print("Failed to write {}.", ini_path().string());
        return 1;
    }
    keymap_config.log_and_print("Wrote {}", ini_path().string());
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
                keymap_config.log_and_print(
                    "Failed to write {}.",
                    ini_path().string()
                );
                return 1;
            }
            keymap_config.log_and_print("Wrote {}", ini_path().string());
            show_settings();
        }
        else if (*line == "2") {
            if (!write_flags({})) {
                keymap_config.log_and_print(
                    "Failed to restore defaults in {}.",
                    ini_path().string()
                );
                return 1;
            }
            keymap_config.log_and_print(
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
    keymap_config.connect_to_logger();
    keymap_config.log_and_log("keymap_config.exe started");

    std::error_code error;
    if (std::filesystem::exists(ini_path(), error)) {
        return configuration_mode();
    }
    if (error) {
        keymap_config.log_and_print("Failed to inspect {}", ini_path().string());
        return 1;
    }
    return first_time();
}
