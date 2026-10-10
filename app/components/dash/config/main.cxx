import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.logging.config;
import auto_core.core.paths;
import components_editor_request;
import config_menu;
import dash_defaults;

import <fstream>;
import <iostream>;
import auto_core.core.shell;

namespace menu = ac::config_menu;

namespace {

    constexpr std::string_view on_off[] {"on", "off"};

    const menu::Setting settings[] {
        {
            .key = "logging",
            .display_name = "Logging",
            .summary = "Controls logging for the Dash component.",
            .default_value = "off",
            .choices = on_off,
        },
    };

    constexpr std::string_view menu_title =
        "Dash Configuration\n\n"
        "The vault is %LOCALAPPDATA%\\Auto Core\\dash.vault.";

    bool logging_in_file(const std::filesystem::path& path) {
        const auto document = ac::ini::read(path);
        if (!document) {
            return false;
        }
        const auto value = document->find("dash", "logging");
        return value && *value == "on";
    }

    bool write_dash_ini(
        ac::Component& log,
        const std::filesystem::path& path,
        const bool logging
    ) {
        std::error_code create_error;
        std::filesystem::create_directories(path.parent_path(), create_error);
        if (create_error) {
            log.log_print(
                "Failed to create config directory: {}",
                create_error.message()
            );
            return false;
        }
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            log.log_print("Failed to create {}", path.string());
            return false;
        }
        const auto contents = menu::with_header(
            settings,
            dash::defaults::ini_for(logging)
        );
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.close();
        if (!output) {
            log.log_print("Failed to write {}", path.string());
            return false;
        }
        return true;
    }

    [[nodiscard]]
    int run_configuration_menu(
        ac::Component& log,
        const std::filesystem::path& path
    ) {
        const auto result = menu::run_menu(
            menu_title,
            settings,
            [&](const menu::Setting&) {
                return logging_in_file(path) ? "on" : "off";
            },
            [&](const menu::Setting&, const std::string_view value) {
                if (!write_dash_ini(log, path, value == "on")) {
                    return menu::ApplyResult::failed;
                }
                log.log_print("Wrote {}", path.string());
                return menu::ApplyResult::stored;
            },
            std::cin,
            std::cout
        );
        return result.ok ? 0 : 1;
    }

    [[nodiscard]]
    int initialize_missing(
        ac::Component& log,
        const std::filesystem::path& path
    ) {
        if (!write_dash_ini(log, path, false)) {
            return 1;
        }
        log.log_print(
            "Wrote default {} (vault path is not configured here).",
            path.string()
        );
        const auto offer = menu::offer_configuration(
            menu_title,
            settings,
            [&](const menu::Setting&) {
                return logging_in_file(path) ? "on" : "off";
            },
            std::cin,
            std::cout
        );
        if (offer == menu::OfferResult::failed) {
            return 1;
        }
        if (offer == menu::OfferResult::configure) {
            return run_configuration_menu(log, path);
        }
        return 0;
    }

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    ac::Component dash_config {
        "dash_config",
        ac::logging::config::LoggingScope {
            "dash",
            ac::logging::config::LoggingFallback::off
        }
    };
    dash_config.log_main("dash_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ac::paths::config_directory() / "dash.ini";
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        dash_config.log_print("Failed to inspect {}", path.string());
        return 1;
    }
    namespace req = ac::config::components_request;
    req::log_config_request(dash_config, *launch);
    if (launch->seed) {
        if (exists) {
            req::log_seed_skipped(dash_config, "config/dash.ini");
            return 0;
        }
        req::log_writing_defaults(dash_config);
        if (!write_dash_ini(dash_config, path, false)) {
            return 1;
        }
        dash_config.log_print(
            "Wrote default {} (vault path is not configured here).",
            path.string()
        );
        req::log_configuration_initialized(dash_config);
        return 0;
    }
    if (launch->init) {
        if (exists) {
            req::log_initialization_skipped(dash_config, "config/dash.ini");
            return 0;
        }
        req::log_configuration_missing(dash_config);
        req::log_writing_defaults(dash_config);
        if (initialize_missing(dash_config, path) != 0) {
            return 1;
        }
        req::log_configuration_initialized(dash_config);
        return 0;
    }
    if (!exists) {
        req::log_configuration_missing(dash_config);
        req::log_writing_defaults(dash_config);
        if (!write_dash_ini(dash_config, path, false)) {
            return 1;
        }
        dash_config.log_print(
            "Wrote {} (vault path is not configured here).",
            path.string()
        );
    }
    return run_configuration_menu(dash_config, path);
}
