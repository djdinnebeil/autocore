import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import slash_defaults;
import slash_config_detail;
import components_editor_request;

import <iostream>;
import auto_core.core.shell;

namespace {

    bool write_ini(
        ac::Component& slash_config,
        const std::filesystem::path& path,
        std::string_view contents
    ) {
        std::error_code create_error;
        std::filesystem::create_directories(path.parent_path(), create_error);
        if (create_error) {
            slash_config.log_print(
                "Failed to create config directory: {}",
                create_error.message()
            );
            return false;
        }
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            slash_config.log_print("Failed to create {}", path.string());
            return false;
        }
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.close();
        if (!output) {
            slash_config.log_print("Failed to write {}", path.string());
            return false;
        }
        slash_config.log_print("Wrote {}", path.string());
        return true;
    }

    std::string mode_in_file(
        ac::Component& slash_config,
        const std::filesystem::path& path
    ) {
        const auto document = ac::ini::read(path);
        if (!document) {
            slash_config.log_print("Failed to read {}", path.string());
            return {};
        }
        const auto mode = document->find("slash", "mode");
        if (mode && slash::defaults::is_mode(*mode)) {
            return std::string {*mode};
        }
        slash_config.log_print(
            "config/slash.ini mode is missing or invalid. Using verbose."
        );
        return std::string {slash::defaults::mode_verbose};
    }

    [[nodiscard]]
    bool logging_in_file(const std::filesystem::path& path) {
        const auto document = ac::ini::read(path);
        if (!document) {
            return ac::logging::config::component_logging_default();
        }
        const auto value = document->find("slash", "logging");
        if (!value) {
            return ac::logging::config::component_logging_default();
        }
        if (*value == "off") {
            return false;
        }
        if (*value == "on") {
            return true;
        }
        return ac::logging::config::component_logging_default();
    }

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    ac::Component slash_config {
        "slash_config",
        ac::logging::config::LoggingScope {"slash"}
    };
    slash_config.log_main("slash_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ac::paths::config_directory() / "slash.ini";
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        slash_config.log_print("Failed to inspect {}", path.string());
        return 1;
    }
    namespace req = ac::config::components_request;
    req::log_config_request(slash_config, *launch);
    const auto action = slash::config::select_action(
        exists,
        launch->init,
        launch->seed
    );
    switch (action) {
        case slash::config::Action::skip_seed:
            req::log_seed_skipped(slash_config, "config/slash.ini");
            return 0;
        case slash::config::Action::skip_initialization:
            req::log_initialization_skipped(slash_config, "config/slash.ini");
            return 0;
        case slash::config::Action::configure: {
            const auto current = mode_in_file(slash_config, path);
            if (current.empty()) {
                return 1;
            }
            const auto mode = slash::config::prompt_mode(
                std::cin,
                std::cout,
                current
            );
            if (!mode) {
                slash_config.log_print("Failed to read mode.");
                return 1;
            }
            const auto logging = slash::config::prompt_logging(
                std::cin,
                std::cout,
                logging_in_file(path)
            );
            if (!logging) {
                return 1;
            }
            if (!write_ini(
                slash_config,
                path,
                slash::defaults::ini_for_mode(*mode, *logging)
            )) {
                return 1;
            }
            return 0;
        }
        case slash::config::Action::seed:
            req::log_writing_defaults(slash_config);
            if (!write_ini(slash_config, path, slash::defaults::ini_text)) {
                return 1;
            }
            req::log_configuration_initialized(slash_config);
            return 0;
        case slash::config::Action::initialize:
            break;
    }
    req::log_configuration_missing(slash_config);
    const auto mode = slash::config::prompt_mode(
        std::cin,
        std::cout,
        slash::defaults::mode_verbose
    );
    if (!mode) {
        slash_config.log_print("Failed to read mode.");
        return 1;
    }
    const auto logging = slash::config::prompt_logging(std::cin, std::cout, true);
    if (!logging) {
        return 1;
    }
    if (!write_ini(
        slash_config,
        path,
        slash::defaults::ini_for_mode(*mode, *logging)
    )) {
        return 1;
    }
    return 0;
}
