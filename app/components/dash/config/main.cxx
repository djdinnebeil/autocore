import std;
import auto_core.core.component;
import auto_core.core.paths;
import dash_defaults;
import components_editor_request;

import <iostream>;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    ac::Component dash_config {"dash_config"};
    dash_config.log_main("dash_config.exe started");

    const auto path = ac::paths::config_directory() / "dash.ini";
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        dash_config.log_print(
            "config/dash.ini already exists.\n"
            "The vault stays under %LOCALAPPDATA%\\Auto Core."
        );
        return ac::config::components_request::run_component_update("dash");
    }

    std::error_code create_error;
    std::filesystem::create_directories(path.parent_path(), create_error);
    if (create_error) {
        dash_config.log_print(
            "Failed to create config directory: {}",
            create_error.message()
        );
        return 1;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        dash_config.log_print("Failed to create {}", path.string());
        return 1;
    }
    output.write(
        dash::defaults::ini_text.data(),
        static_cast<std::streamsize>(dash::defaults::ini_text.size())
    );
    output.close();
    if (!output) {
        dash_config.log_print("Failed to write {}", path.string());
        return 1;
    }
    dash_config.log_print(
        "Wrote default {} (vault path is not configured here).",
        path.string()
    );
    return ac::config::components_request::run_component_update("dash");
}
