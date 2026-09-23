import std;
import auto_core.core.component;
import auto_core.core.paths;
import wake_defaults;
import components_editor_request;

import <iostream>;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    ac::Component wake_config {"wake_config"};
    wake_config.connect_to_logger();
    wake_config.log_and_log("wake_config.exe started");

    const auto path = ac::paths::config_directory() / "wake.ini";
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        wake_config.log_and_print(
            "config/wake.ini already exists. Wake has no extra settings."
        );
        return ac::config::components_request::run_component_update("wake");
    }

    std::error_code create_error;
    std::filesystem::create_directories(path.parent_path(), create_error);
    if (create_error) {
        wake_config.log_and_print(
            "Failed to create config directory: {}",
            create_error.message()
        );
        return 1;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        wake_config.log_and_print("Failed to create {}", path.string());
        return 1;
    }
    output.write(
        wake::defaults::ini_text.data(),
        static_cast<std::streamsize>(wake::defaults::ini_text.size())
    );
    output.close();
    if (!output) {
        wake_config.log_and_print("Failed to write {}", path.string());
        return 1;
    }
    wake_config.log_and_print(
        "Wrote default {} (no tunables yet).",
        path.string()
    );
    return ac::config::components_request::run_component_update("wake");
}
