#include "../../../core/component/components_catalog_detail.hpp"

import std;
import auto_core.core.component;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import components_editor_request;
import config_menu;

import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;
namespace catalog = ac::config::components_catalog;
namespace menu = ac::config_menu;

namespace {

ac::Component components_config {"components_config"};

constexpr std::string_view new_component_values[] {"prompt", "on", "off"};
constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting menu_settings[] {
    {
        .key = "new_components",
        .display_name = "New components",
        .summary = "How a newly discovered component is added to the list.",
        .default_value = defaults::components_new_components,
        .choices = new_component_values,
    },
    {
        .key = "sort_components",
        .display_name = "Sort components",
        .summary = "Sort the component list when it is rebuilt.",
        .default_value = "on",
        .choices = on_off,
    },
    {
        .key = "remove_missing_components",
        .display_name = "Remove missing components",
        .summary = "Drop list entries whose component is no longer installed.",
        .default_value = "off",
        .choices = on_off,
    },
};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "components.ini";
}

[[nodiscard]]
catalog::Settings default_settings() {
    catalog::Settings settings;
    settings.new_components =
        catalog::parse_new_components(defaults::components_new_components);
    settings.sort_components = defaults::components_sort;
    settings.remove_missing_components = defaults::components_remove_missing;
    return settings;
}

[[nodiscard]]
bool write_settings(const catalog::Settings& settings) {
    if (!cfg::ensure_directory(ac::paths::config_directory())) {
        return false;
    }
    if (!cfg::write_bytes(
            ini_path(),
            menu::with_header(menu_settings, catalog::format_settings(settings))
        )) {
        return false;
    }
    components_config.log_print("Wrote {}", ini_path().string());
    return true;
}

[[nodiscard]]
bool file_exists(const std::filesystem::path& path, std::error_code& error) {
    return std::filesystem::exists(path, error);
}

[[nodiscard]]
std::optional<std::string> read_text(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        components_config.log_print("Failed to read {}", path.string());
        return std::nullopt;
    }
    return std::string {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
}

[[nodiscard]]
std::optional<catalog::Settings> load_settings() {
    std::error_code error;
    const bool ini_present = file_exists(ini_path(), error);
    if (error) {
        components_config.log_print(
            "Failed to inspect {}",
            ini_path().string()
        );
        return std::nullopt;
    }
    if (!ini_present) {
        return default_settings();
    }
    const auto text = read_text(ini_path());
    if (!text) {
        return std::nullopt;
    }
    return catalog::parse_settings(*text);
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    const auto loaded = load_settings();
    const auto values = loaded ? *loaded : default_settings();
    if (setting.key == "new_components") {
        return std::string {catalog::to_string(values.new_components)};
    }
    if (setting.key == "sort_components") {
        return values.sort_components ? "on" : "off";
    }
    return values.remove_missing_components ? "on" : "off";
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    auto loaded = load_settings();
    if (!loaded) {
        return menu::ApplyResult::failed;
    }
    if (setting.key == "new_components") {
        loaded->new_components = catalog::parse_new_components(value);
    }
    else if (setting.key == "sort_components") {
        loaded->sort_components = value == "on";
    }
    else {
        loaded->remove_missing_components = value == "on";
    }
    if (!write_settings(*loaded)) {
        return menu::ApplyResult::failed;
    }
    return menu::ApplyResult::stored;
}

[[nodiscard]]
menu::MenuResult run_configuration_menu() {
    return menu::run_menu(
        "Components Configuration",
        menu_settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
}

[[nodiscard]]
int offer_sync() {
    const auto run_sync = cfg::prompt_on_off("Run components_editor.exe", true);
    if (!run_sync) {
        return 1;
    }
    if (!*run_sync) {
        return 0;
    }
    std::cout << "Running components_editor.exe...\n";
    if (cfg::run_config_exe("components_editor.exe") != 0) {
        components_config.log_print(
            "components_editor.exe did not complete successfully."
        );
        return 1;
    }
    return 0;
}

[[nodiscard]]
int initialize_missing() {
    if (!write_settings(default_settings())) {
        return 1;
    }
    const auto offer = menu::offer_configuration(
        "Components Configuration",
        menu_settings,
        current_text,
        std::cin,
        std::cout
    );
    if (offer == menu::OfferResult::failed) {
        return 1;
    }
    if (offer == menu::OfferResult::configure) {
        const auto result = run_configuration_menu();
        return result.ok ? 0 : 1;
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    components_config.log_main("components_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    std::error_code error;
    const bool ini_present = std::filesystem::exists(ini_path(), error);
    if (error) {
        components_config.log_print(
            "Failed to inspect {}",
            ini_path().string()
        );
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(components_config, *launch);
    if (launch->seed) {
        if (ini_present) {
            req::log_seed_skipped(components_config, "config/components.ini");
            return 0;
        }
        req::log_writing_defaults(components_config);
        if (!write_settings(default_settings())) {
            return 1;
        }
        req::log_configuration_initialized(components_config);
        return 0;
    }

    if (launch->init) {
        if (ini_present) {
            req::log_initialization_skipped(
                components_config,
                "config/components.ini"
            );
            return 0;
        }
        req::log_configuration_missing(components_config);
        req::log_writing_defaults(components_config);
        if (initialize_missing() != 0) {
            return 1;
        }
        req::log_configuration_initialized(components_config);
        return 0;
    }

    if (!ini_present) {
        req::log_configuration_missing(components_config);
        req::log_writing_defaults(components_config);
        if (!write_settings(default_settings())) {
            return 1;
        }
    }
    const auto result = run_configuration_menu();
    if (!result.ok) {
        return 1;
    }
    if (!ini_present || result.changed) {
        return offer_sync();
    }
    return 0;
}
