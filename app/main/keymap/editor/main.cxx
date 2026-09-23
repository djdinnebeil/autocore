#include "../../component/src/keymap_map_detail.hpp"

import std;
import auto_core.core.component;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.key_codes;

namespace cfg = ac::main::config;

namespace {

ac::Component keymap_editor {"keymap_editor"};

[[nodiscard]]
std::string mappings_seed() {
    std::string contents =
        "# Key binding: key = primary | secondary\n"
        "# Example: numpad_1 = activate_auto_core | close_program\n"
        "# Leave a binding blank to return control to the OS\n"
        "\n"
        "[keymap: concise]\n";
    for (const auto& key : key_codes::keys) {
        if (key.name == "numpad_0") {
            contents += ac::keymap::map_file::format_line(
                key.name,
                "activate_function_key",
                "deactivate_function_key"
            );
        }
        else if (key.name == "numpad_1") {
            contents += ac::keymap::map_file::format_line(
                key.name,
                "activate_auto_core",
                "close_program"
            );
        }
        else {
            contents += ac::keymap::map_file::format_line(key.name, {}, {});
        }
    }
    return contents;
}

} // namespace

int main() {
    keymap_editor.connect_to_logger();
    keymap_editor.log_and_log("keymap_editor.exe started");

    const auto path = ac::paths::keymap_file();
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        return 0;
    }
    if (error) {
        keymap_editor.log_and_print("Failed to inspect {}", path.string());
        return 1;
    }
    if (!cfg::write_bytes(path, mappings_seed())) {
        keymap_editor.log_and_print("Failed to write {}.", path.string());
        return 1;
    }
    keymap_editor.log_and_print("Wrote {}", path.string());
    return 0;
}
