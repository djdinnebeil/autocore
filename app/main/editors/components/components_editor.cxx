/**
 * \file components_editor.cxx
 * \brief Sole writer of components.list.
 *
 * `--edit` full-synchronizes the list. `--edit --seed` creates it only when
 * missing. `--enable` and `--disable` call `set_component_listed_state`.
 */
#include "components_list_publish.hpp"
#include "components_usage.hpp"

#include "../../../core/component/components_catalog_detail.hpp"
#include "../../../core/component/components_list_detail.hpp"

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
namespace publish = ac::components_list_publish;

namespace {

ac::Component components_editor {"components_editor", "auto_core"};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "components_policy.ini";
}

[[nodiscard]]
std::filesystem::path list_path() {
    return ac::paths::components_list_file();
}

[[nodiscard]]
std::filesystem::path component_ini_path(const std::string_view name) {
    if (name == list::writer_name) {
        return ac::paths::writer_config_file();
    }
    return ac::paths::component_config_file(name);
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
std::filesystem::path lock_path() {
    return publish::list_lock_file(ac::paths::installation_root());
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
        components_editor.print("Failed to read {}", path.string());
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
        components_editor.print(
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
        components_editor.print(
            "Failed to inspect {}",
            list_path().string()
        );
        return std::nullopt;
    }
    if (!list_present) {
        return document;
    }
    const auto text = list::read_list_bytes(list_path());
    if (text.status != list::ListReadStatus::ok) {
        components_editor.print("Failed to read {}", list_path().string());
        return std::nullopt;
    }
    auto listed = catalog::parse_list(text.bytes);
    document.components = std::move(listed.components);
    document.settings = *settings;
    return document;
}

[[nodiscard]]
bool is_log_merger(const std::string_view name) noexcept {
    return name == list::log_merger_name;
}

/**
 * \brief `*_ac.exe` names, plus `log_merger` when it can be offered or is saved.
 *
 * The helper offers a new line. A saved line stays in this vector when the
 * helper is missing, so `remove_missing_components` does not drop it.
 */
[[nodiscard]]
std::vector<std::string> sync_names(const catalog::Document& document) {
    auto names = list::discover_ac_executables(
        ac::paths::component_executable_directory()
    );
    const bool saved = catalog::listed(document, list::log_merger_name);
    const bool offer = list::log_merger_helper_present(
        ac::paths::app_directory()
    );
    if (saved || offer) {
        list::record_unique(names, list::log_merger_name);
    }
    list::record_writer_if_present(names, ac::paths::app_directory());
    return names;
}

[[nodiscard]]
std::vector<std::string> names_needing_prompt(
    const catalog::Document& document,
    const std::vector<std::string>& names,
    const std::map<std::string, bool>& saved
) {
    std::vector<std::string> prompts;
    if (document.settings.new_components !=
        catalog::NewComponentsPolicy::prompt) {
        return prompts;
    }
    for (const auto& name : names) {
        if (catalog::listed(document, name) ||
            saved.contains(name) ||
            (!is_log_merger(name) && !component_ini_exists(name))) {
            continue;
        }
        prompts.push_back(name);
    }
    return prompts;
}

[[nodiscard]]
bool apply_new_names(
    catalog::Document& document,
    const std::vector<std::string>& names,
    const std::map<std::string, bool>& saved
) {
    for (const auto& name : names) {
        if (catalog::listed(document, name) ||
            (!is_log_merger(name) && !component_ini_exists(name))) {
            continue;
        }
        bool enabled = false;
        switch (document.settings.new_components) {
        case catalog::NewComponentsPolicy::on:
            enabled = true;
            break;
        case catalog::NewComponentsPolicy::off:
            enabled = false;
            break;
        case catalog::NewComponentsPolicy::prompt:
        default: {
            const auto answer = saved.find(name);
            if (answer == saved.end()) {
                return false;
            }
            enabled = answer->second;
            break;
        }
        }
        catalog::add_if_missing(
            document,
            name,
            [&](std::string_view) { return enabled; }
        );
    }
    return true;
}

[[nodiscard]]
bool publish_document(const catalog::Document& document) {
    const auto published = publish::publish_list(
        list_path(),
        catalog::format_list(document)
    );
    if (!published.ok) {
        components_editor.print("{}", published.message);
        return false;
    }
    components_editor.log_debug("Wrote {}", list_path().string());
    return true;
}

[[nodiscard]]
int synchronize_list(const bool seed) {
    std::map<std::string, bool> saved;
    while (true) {
        std::vector<std::string> prompts;
        {
            publish::ListUpdateLock lock;
            std::string error;
            if (!lock.acquire(lock_path(), error)) {
                components_editor.print("{}", error);
                return 1;
            }

            std::error_code exists_error;
            const bool list_present = file_exists(list_path(), exists_error);
            if (exists_error) {
                components_editor.print(
                    "Failed to inspect {}",
                    list_path().string()
                );
                return 1;
            }
            if (seed && list_present) {
                components_editor.log_debug("components.list already exists.");
                return 0;
            }

            auto document = load_document();
            if (!document) {
                return 1;
            }
            const auto names = sync_names(*document);
            prompts = names_needing_prompt(*document, names, saved);
            if (prompts.empty()) {
                catalog::remove_missing(*document, names);
                if (!apply_new_names(*document, names, saved)) {
                    return 1;
                }
                catalog::apply_sort(*document);
                if (!publish_document(*document)) {
                    return 1;
                }
                return 0;
            }
        }

        for (const auto& name : prompts) {
            const auto enabled = cfg::prompt_on_off(name, true);
            if (!enabled) {
                return 1;
            }
            saved.insert_or_assign(name, *enabled);
        }
    }
}

[[nodiscard]]
int set_explicit_state(const std::string_view name, const bool enabled) {
    if (!list::is_valid_component_name(name)) {
        components_editor.print(
            "Invalid component name: {}",
            name
        );
        return 1;
    }
    if (is_log_merger(name)) {
        if (enabled &&
            !list::log_merger_helper_present(ac::paths::app_directory())) {
            components_editor.print(
                "auto_core_logger.exe is missing. components.list was not modified."
            );
            return 1;
        }
    }
    else if (!component_ini_exists(name)) {
        if (name == list::writer_name) {
            components_editor.print(
                "config/writer.ini is missing. components.list was not modified."
            );
        }
        else {
            components_editor.print(
                "config/components/{}.ini is missing. components.list was not modified.",
                name
            );
        }
        return 1;
    }

    publish::ListUpdateLock lock;
    std::string error;
    if (!lock.acquire(lock_path(), error)) {
        components_editor.print("{}", error);
        return 1;
    }

    const auto text = list::read_list_bytes(list_path());
    if (text.status == list::ListReadStatus::not_found) {
        components_editor.print(
            "components.list is missing. It was not modified."
        );
        return 1;
    }
    if (text.status != list::ListReadStatus::ok) {
        components_editor.print("Failed to read {}", list_path().string());
        return 1;
    }
    const auto updated = catalog::set_listed_state(text.bytes, name, enabled);
    if (!updated) {
        components_editor.print(
            "Invalid component name: {}",
            name
        );
        return 1;
    }
    const auto published = publish::publish_list(list_path(), *updated);
    if (!published.ok) {
        components_editor.print("{}", published.message);
        return 1;
    }
    components_editor.log_debug("Wrote {}", list_path().string());
    return 0;
}

} // namespace

int set_component_listed_state(
    const std::string_view name,
    const bool enabled
) {
    components_editor.log_info(
        enabled
            ? "auto_core_components.exe --enable started"
            : "auto_core_components.exe --disable started"
    );
    return set_explicit_state(name, enabled);
}

int run_components_editor(int argc, char* argv[]) {
    bool seed = false;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument {argv[index]};
        if (argument == "--seed") {
            if (seed) {
                std::cerr << "Unknown argument: " << argument << '\n';
                print_components_usage();
                return 1;
            }
            seed = true;
            continue;
        }
        std::cerr << "Unknown argument: " << argument << '\n';
        print_components_usage();
        return 1;
    }

    components_editor.log_info(
        seed
            ? "auto_core_components.exe --edit --seed started"
            : "auto_core_components.exe --edit started"
    );
    return synchronize_list(seed);
}
