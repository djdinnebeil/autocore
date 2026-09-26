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
    components_editor.log_print("Wrote {}", list_path().string());
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
        components_editor.log_print("Failed to read {}", path.string());
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
        components_editor.log_print(
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
        components_editor.log_print(
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
int set_explicit_state(const std::string_view name, const bool enabled) {
    if (!list::is_valid_component_name(name)) {
        components_editor.log_print(
            "Invalid component name: {}",
            name
        );
        return 1;
    }
    if (!component_ini_exists(name)) {
        components_editor.log_print(
            "config/{}.ini is missing. components.list was not modified.",
            name
        );
        return 1;
    }

    std::error_code error;
    const bool list_present = file_exists(list_path(), error);
    if (error) {
        components_editor.log_print(
            "Failed to inspect {}",
            list_path().string()
        );
        return 1;
    }
    if (!list_present) {
        components_editor.log_print(
            "components.list is missing. It was not modified."
        );
        return 1;
    }

    const auto text = read_text(list_path());
    if (!text) {
        return 1;
    }
    const auto updated = catalog::set_listed_state(*text, name, enabled);
    if (!updated) {
        components_editor.log_print(
            "Invalid component name: {}",
            name
        );
        return 1;
    }
    if (!cfg::write_bytes(list_path(), *updated)) {
        return 1;
    }
    components_editor.log_print("Wrote {}", list_path().string());
    return 0;
}

[[nodiscard]]
int single_component_update(const std::string_view name) {
    if (!list::is_valid_component_name(name)) {
        components_editor.log_print(
            "Invalid component name: {}",
            name
        );
        return 1;
    }
    if (!component_ini_exists(name)) {
        components_editor.log_print(
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

struct EditorArguments {
    std::string component;
    std::optional<bool> enabled;
};

[[nodiscard]]
std::optional<EditorArguments> parse_arguments(const int argc, char* argv[]) {
    EditorArguments arguments;
    bool saw_on = false;
    bool saw_off = false;

    for (int i = 1; i < argc; ++i) {
        const std::string_view argument {argv[i]};
        if (argument == "--component") {
            if (i + 1 >= argc) {
                std::cerr << "Missing component name after --component.\n";
                return std::nullopt;
            }
            arguments.component = argv[++i];
            continue;
        }
        if (argument.starts_with("--component=")) {
            const auto name = argument.substr(12);
            if (name.empty()) {
                std::cerr << "Missing component name after --component.\n";
                return std::nullopt;
            }
            arguments.component = std::string {name};
            continue;
        }
        if (argument == "--on") {
            saw_on = true;
            continue;
        }
        if (argument == "--off") {
            saw_off = true;
            continue;
        }
        std::cerr << "Unknown argument: " << argument << '\n';
        return std::nullopt;
    }

    if (saw_on && saw_off) {
        std::cerr << "--on and --off cannot be used together.\n";
        return std::nullopt;
    }
    if ((saw_on || saw_off) && arguments.component.empty()) {
        std::cerr << "Missing component name for --on or --off.\n";
        return std::nullopt;
    }
    if (saw_on) {
        arguments.enabled = true;
    }
    else if (saw_off) {
        arguments.enabled = false;
    }
    return arguments;
}

} // namespace

int main(int argc, char* argv[]) {
    components_editor.log_main("components_editor.exe started");

    const auto arguments = parse_arguments(argc, argv);
    if (!arguments) {
        return 1;
    }
    if (arguments->enabled) {
        return set_explicit_state(
            arguments->component,
            *arguments->enabled
        );
    }
    if (!arguments->component.empty()) {
        return single_component_update(arguments->component);
    }
    return full_synchronize();
}
