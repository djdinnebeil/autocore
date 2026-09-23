import std;
import auto_core.core.component;
import auto_core.core.paths;
import logger_defaults;
import components_editor_request;

import <iostream>;

namespace {

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool write_bytes(
    ac::Component& log,
    const std::filesystem::path& path,
    std::string_view contents
) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        log.log_and_print(
            "Failed to create config directory: {}",
            error.message()
        );
        return false;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        log.log_and_print("Failed to create {}", path.string());
        return false;
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    return static_cast<bool>(output);
}

} // namespace

int main(int argc, char* argv[]) {
    ac::Component logger_config {"logger_config"};
    logger_config.connect_to_logger();
    logger_config.log_and_log("logger_config.exe started");

    const bool initialize =
        ac::config::components_request::is_initialize_run(argc, argv);
    const auto path = ac::paths::config_directory() / "logger.ini";
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        logger_config.log_and_print("config/logger.ini already exists.");
        return ac::config::components_request::run_component_update("logger");
    }

    if (initialize) {
        if (!write_bytes(logger_config, path, logger::defaults::ini_text)) {
            return 1;
        }
        logger_config.log_and_print("Wrote {}", path.string());
        return ac::config::components_request::run_component_update("logger");
    }

    std::cout << "Log directory [." << '\\' << logger::defaults::directory << "]: ";
    std::string directory_input;
    if (!std::getline(std::cin, directory_input)) {
        logger_config.log_and_print("Failed to read log directory.");
        return 1;
    }
    const auto directory = trim(directory_input);

    std::cout << "write_to_console [false]: ";
    std::string console_input;
    if (!std::getline(std::cin, console_input)) {
        logger_config.log_and_print("Failed to read write_to_console.");
        return 1;
    }
    bool console = logger::defaults::write_to_console;
    const auto console_value = trim(console_input);
    if (console_value == "true" || console_value == "on") {
        console = true;
    }
    else if (console_value == "false" || console_value == "off" ||
        console_value.empty()) {
        console = false;
    }
    else {
        std::cout << "Enter true/false or on/off. Using default.\n";
    }

    if (!write_bytes(
            logger_config,
            path,
            logger::defaults::ini_for(
                directory.empty() ? logger::defaults::directory : directory,
                console
            )
        )) {
        return 1;
    }
    logger_config.log_and_print("Wrote {}", path.string());
    return ac::config::components_request::run_component_update("logger");
}
