#include "../../../core/src/components_catalog_detail.hpp"
#include "../../../core/src/components_list_detail.hpp"

import std;
import auto_core.core.component;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;

import <iostream>;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;
namespace catalog = ac::config::components_catalog;
namespace list = ac::config::components_list;

namespace {

ac::Component components_config {"components_config"};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "components.ini";
}

[[nodiscard]]
std::filesystem::path component_ini_path(const std::string_view name) {
    return ac::paths::config_directory() / (std::string {name} + ".ini");
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
    if (!cfg::write_bytes(ini_path(), catalog::format_settings(settings))) {
        return false;
    }
    components_config.log_and_print("Wrote {}", ini_path().string());
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
        components_config.log_and_print("Failed to read {}", path.string());
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
        components_config.log_and_print(
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
std::vector<std::string> discovered_names() {
    return list::discover_ac_executables(ac::paths::bin_directory());
}

[[nodiscard]]
std::optional<catalog::Settings> prompt_settings(
    const catalog::Settings& suggestion
) {
    const auto new_components = cfg::prompt_choice(
        "new_components",
        catalog::to_string(suggestion.new_components),
        {"prompt", "on", "off"}
    );
    if (!new_components) {
        return std::nullopt;
    }
    const auto sort_components = cfg::prompt_on_off(
        "sort_components",
        suggestion.sort_components
    );
    if (!sort_components) {
        return std::nullopt;
    }
    const auto remove_missing = cfg::prompt_on_off(
        "remove_missing_components",
        suggestion.remove_missing_components
    );
    if (!remove_missing) {
        return std::nullopt;
    }
    catalog::Settings settings;
    settings.new_components = catalog::parse_new_components(*new_components);
    settings.sort_components = *sort_components;
    settings.remove_missing_components = *remove_missing;
    return settings;
}

void show_settings(const catalog::Settings& settings) {
    std::cout
        << "Current config/components.ini\n"
        << "  new_components = "
        << catalog::to_string(settings.new_components)
        << "\n  sort_components = "
        << (settings.sort_components ? "on" : "off")
        << "\n  remove_missing_components = "
        << (settings.remove_missing_components ? "on" : "off")
        << '\n';
}

void initialize_missing_component_inis() {
    for (const auto& name : discovered_names()) {
        std::error_code error;
        const auto path = component_ini_path(name);
        if (file_exists(path, error)) {
            continue;
        }
        if (error) {
            components_config.log_and_print(
                "Failed to inspect {}",
                path.string()
            );
            continue;
        }
        const auto helper = std::string {name} + "_config.exe";
        std::cout << "config/" << name << ".ini is missing. Launching "
                  << helper << ".\n";
        if (cfg::run_config_exe(helper, L"--initialize") != 0) {
            components_config.log_and_print(
                "{} did not complete successfully.",
                helper
            );
        }
    }
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
        components_config.log_and_print(
            "components_editor.exe did not complete successfully."
        );
        return 1;
    }
    return 0;
}

[[nodiscard]]
int after_settings_written() {
    initialize_missing_component_inis();
    return offer_sync();
}

[[nodiscard]]
int initialize_files() {
    std::cout
        << "Component configuration is missing. Create it using the defaults.\n";
    const auto settings = prompt_settings(default_settings());
    if (!settings) {
        return 1;
    }
    if (!write_settings(*settings)) {
        return 1;
    }
    return after_settings_written();
}

[[nodiscard]]
int configuration_mode() {
    auto settings = load_settings();
    if (!settings) {
        return 1;
    }
    if (after_settings_written() != 0) {
        return 1;
    }
    settings = load_settings();
    if (!settings) {
        return 1;
    }
    show_settings(*settings);
    while (true) {
        std::cout
            << "\ncomponents_config\n"
            << "  1. Modify settings\n"
            << "  2. Restore defaults\n"
            << "  3. Exit\n"
            << "Choice: ";
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        if (*line == "1") {
            const auto updated = prompt_settings(*settings);
            if (!updated) {
                return 1;
            }
            if (!write_settings(*updated)) {
                return 1;
            }
            if (after_settings_written() != 0) {
                return 1;
            }
            settings = load_settings();
            if (!settings) {
                return 1;
            }
            show_settings(*settings);
        }
        else if (*line == "2") {
            if (!write_settings(default_settings())) {
                return 1;
            }
            if (after_settings_written() != 0) {
                return 1;
            }
            settings = load_settings();
            if (!settings) {
                return 1;
            }
            components_config.log_and_print(
                "Restored defaults in {}",
                ini_path().string()
            );
            show_settings(*settings);
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

int main(int argc, char* argv[]) {
    components_config.connect_to_logger();
    components_config.log_and_log("components_config.exe started");

    if (argc > 1) {
        components_config.log_and_print("Unknown argument: {}", argv[1]);
        return 1;
    }

    std::error_code error;
    const bool ini_present = std::filesystem::exists(ini_path(), error);
    if (error) {
        components_config.log_and_print(
            "Failed to inspect {}",
            ini_path().string()
        );
        return 1;
    }
    if (ini_present) {
        return configuration_mode();
    }
    return initialize_files();
}
