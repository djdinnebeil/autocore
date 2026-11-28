/**
 * \file settings_catalog.hpp
 * \brief Discovery and menu rows for `auto_core_settings.exe`.
 *
 * Installed `*_ac.exe` files are the component rows. This scan does not
 * read `components.list` and does not treat `*_settings.exe` as a component.
 * The Auto Core row launches this executable with `--config`. Each component
 * row launches `<name>_ac.exe --settings`. Enablement stays in
 * `components.list`.
 */
#pragma once

#include "../../core/component/components_list_detail.hpp"
#include "../shared/provider_display_name.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ac::main::settings {

    namespace list = ac::config::components_list;

    struct MenuRow {
        std::string label;
        std::string executable;
        std::vector<std::string> arguments {};
        /** When true, `executable` is resolved under the app directory. */
        bool in_app_directory {false};
    };

    [[nodiscard]]
    inline std::string component_display_name(const std::string_view identity) {
        return provider_display_name(identity);
    }

    [[nodiscard]]
    inline std::string component_row_label(const std::string_view identity) {
        std::string label = "Modify ";
        label.append(component_display_name(identity));
        label.append(" settings");
        return label;
    }

    [[nodiscard]]
    inline std::vector<std::string> discover_settings_executables(
        const std::filesystem::path& directory,
        const std::filesystem::path& app_directory = {}
    ) {
        auto names = list::discover_ac_executables(directory);
        if (!app_directory.empty()) {
            list::record_writer_if_present(names, app_directory);
        }
        return names;
    }

    [[nodiscard]]
    inline std::vector<MenuRow> menu_rows(
        const std::vector<std::string>& identities
    ) {
        std::vector<MenuRow> rows;
        rows.push_back({
            "Modify Auto Core settings",
            "auto_core_settings.exe",
            {"--config"}
        });
        rows.push_back({
            "Modify Taskbar settings",
            "auto_core_taskbar.exe",
            {"--settings"},
            true
        });
        for (const auto& identity : identities) {
            if (identity == list::writer_name) {
                rows.push_back({
                    component_row_label(identity),
                    "auto_core_writer.exe",
                    {"--settings"},
                    true
                });
                continue;
            }
            rows.push_back({
                component_row_label(identity),
                identity + "_ac.exe",
                {"--settings"}
            });
        }
        return rows;
    }

} // namespace ac::main::settings
