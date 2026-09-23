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

ac::Component components_editor {"components_editor"};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "components.ini";
}

[[nodiscard]]
std::filesystem::path list_path() {
    return ac::paths::components_list_file();
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
bool write_list(const catalog::Document& document) {
    if (!cfg::write_bytes(list_path(), catalog::format_list(document))) {
        return false;
    }
    components_editor.log_and_print("Wrote {}", list_path().string());
    return true;
}

[[nodiscard]]
bool file_exists(const std::filesystem::path& path, std::error_code& error) {
    return std::filesystem::exists(path, error);
}

[[nodiscard]]
bool component_ini_exists(const std::string_view name) {
    std::error_code error;
    const bool present = file_exists(component_ini_path(name), error);
    return !error && present;
}

[[nodiscard]]
std::optional<std::string> read_text(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        components_editor.log_and_print("Failed to read {}", path.string());
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
        components_editor.log_and_print(
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
std::optional<catalog::Document> load_document() {
    const auto settings = load_settings();
    if (!settings) {
        return std::nullopt;
    }
    catalog::Document document;
    document.settings = *settings;

    std::error_code error;
    const bool list_present = file_exists(list_path(), error);
    if (error) {
        components_editor.log_and_print(
            "Failed to inspect {}",
            list_path().string()
        );
        return std::nullopt;
    }
    if (!list_present) {
        return document;
    }
    const auto text = read_text(list_path());
    if (!text) {
        return std::nullopt;
    }
    auto listed = catalog::parse_list(*text);
    document.components = std::move(listed.components);
    document.settings = *settings;
    return document;
}

[[nodiscard]]
std::vector<std::string> discovered_names() {
    return list::discover_ac_executables(ac::paths::bin_directory());
}

[[nodiscard]]
std::optional<bool> resolve_new_enabled(
    const std::string_view name,
    const catalog::NewComponentsPolicy policy
) {
    switch (policy) {
    case catalog::NewComponentsPolicy::on:
        return true;
    case catalog::NewComponentsPolicy::off:
        return false;
    case catalog::NewComponentsPolicy::prompt:
    default:
        return cfg::prompt_on_off(name, true);
    }
}

[[nodiscard]]
bool apply_new_names(
    catalog::Document& document,
    const std::vector<std::string>& names
) {
    for (const auto& name : names) {
        if (catalog::listed(document, name)) {
            continue;
        }
        if (!component_ini_exists(name)) {
            continue;
        }
        const auto enabled = resolve_new_enabled(
            name,
            document.settings.new_components
        );
        if (!enabled) {
            return false;
        }
        catalog::add_if_missing(
            document,
            name,
            [&](std::string_view) { return *enabled; }
        );
    }
    return true;
}

[[nodiscard]]
int full_synchronize() {
    auto document = load_document();
    if (!document) {
        return 1;
    }
    catalog::remove_missing(*document, discovered_names());
    if (!apply_new_names(*document, discovered_names())) {
        return 1;
    }
    catalog::apply_sort(*document);
    if (!write_list(*document)) {
        return 1;
    }
    return 0;
}

[[nodiscard]]
int single_component_update(const std::string_view name) {
    if (!list::is_valid_component_name(name)) {
        components_editor.log_and_print(
            "Invalid component name: {}",
            name
        );
        return 1;
    }
    if (!component_ini_exists(name)) {
        components_editor.log_and_print(
            "config/{}.ini is missing. components.list was not modified.",
            name
        );
        return 1;
    }

    auto document = load_document();
    if (!document) {
        return 1;
    }
    if (!apply_new_names(*document, {std::string {name}})) {
        return 1;
    }
    catalog::apply_sort(*document);
    if (!write_list(*document)) {
        return 1;
    }
    return 0;
}

[[nodiscard]]
std::optional<std::string> parse_component_argument(
    const int argc,
    char* argv[]
) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument {argv[i]};
        if (argument == "--component") {
            if (i + 1 >= argc) {
                std::cerr << "Missing component name after --component.\n";
                return std::nullopt;
            }
            return std::string {argv[i + 1]};
        }
        if (argument.starts_with("--component=")) {
            auto name = argument.substr(12);
            if (name.empty()) {
                std::cerr << "Missing component name after --component.\n";
                return std::nullopt;
            }
            return std::string {name};
        }
        std::cerr << "Unknown argument: " << argument << '\n';
        return std::nullopt;
    }
    return std::string {};
}

} // namespace

int main(int argc, char* argv[]) {
    components_editor.connect_to_logger();
    components_editor.log_and_log("components_editor.exe started");

    const auto component = parse_component_argument(argc, argv);
    if (!component) {
        return 1;
    }
    if (!component->empty()) {
        return single_component_update(*component);
    }
    return full_synchronize();
}
