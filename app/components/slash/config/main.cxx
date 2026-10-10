import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import components_editor_request;
import config_menu;
import slash_config_detail;
import slash_defaults;

import <iostream>;
import auto_core.core.shell;

namespace menu = ac::config_menu;

namespace {

    constexpr std::string_view modes[] {"verbose", "concise", "silent"};
    constexpr std::string_view on_off[] {"on", "off"};

    const menu::Setting settings[] {
        {
            .key = "mode",
            .display_name = "Mode",
            .summary =
                "verbose is the categorized report. concise reports the item "
                "count. silent prints one line and does not insert text.",
            .default_value = slash::defaults::mode_verbose,
            .choices = modes,
        },
        {
            .key = "logging",
            .display_name = "Logging",
            .summary = "Controls logging for the Slash component.",
            .default_value = "on",
            .choices = on_off,
        },
    };

    bool write_ini(
        ac::Component& slash_config,
        const std::filesystem::path& path,
        const std::string_view mode,
        const bool logging
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
        const auto contents = menu::with_header(
            settings,
            slash::defaults::ini_for_mode(mode, logging)
        );
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
        if (!value || (*value != "on" && *value != "off")) {
            return ac::logging::config::component_logging_default();
        }
        return *value == "on";
    }

    [[nodiscard]]
    int run_configuration_menu(
        ac::Component& slash_config,
        const std::filesystem::path& path
    ) {
        const auto result = menu::run_menu(
            "Slash Configuration",
            settings,
            [&](const menu::Setting& setting) {
                if (setting.key == "mode") {
                    return mode_in_file(slash_config, path);
                }
                return std::string {logging_in_file(path) ? "on" : "off"};
            },
            [&](const menu::Setting& setting, const std::string_view value) {
                const auto mode = setting.key == "mode"
                    ? std::string {value}
                    : mode_in_file(slash_config, path);
                if (mode.empty()) {
                    return menu::ApplyResult::failed;
                }
                const bool logging = setting.key == "logging"
                    ? value == "on"
                    : logging_in_file(path);
                if (!write_ini(slash_config, path, mode, logging)) {
                    return menu::ApplyResult::failed;
                }
                return menu::ApplyResult::stored;
            },
            std::cin,
            std::cout
        );
        return result.ok ? 0 : 1;
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
        case slash::config::Action::seed:
            req::log_writing_defaults(slash_config);
            if (!write_ini(
                    slash_config,
                    path,
                    slash::defaults::mode_verbose,
                    true
                )) {
                return 1;
            }
            req::log_configuration_initialized(slash_config);
            return 0;
        case slash::config::Action::configure:
            return run_configuration_menu(slash_config, path);
        case slash::config::Action::initialize:
            break;
    }

    req::log_configuration_missing(slash_config);
    req::log_writing_defaults(slash_config);
    if (!write_ini(slash_config, path, slash::defaults::mode_verbose, true)) {
        return 1;
    }
    if (launch->init) {
        const auto offer = menu::offer_configuration(
            "Slash Configuration",
            settings,
            [&](const menu::Setting& setting) {
                if (setting.key == "mode") {
                    return mode_in_file(slash_config, path);
                }
                return std::string {logging_in_file(path) ? "on" : "off"};
            },
            std::cin,
            std::cout
        );
        if (offer == menu::OfferResult::failed) {
            return 1;
        }
        if (offer == menu::OfferResult::leave) {
            req::log_configuration_initialized(slash_config);
            return 0;
        }
    }
    if (run_configuration_menu(slash_config, path) != 0) {
        return 1;
    }
    req::log_configuration_initialized(slash_config);
    return 0;
}
