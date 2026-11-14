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
    };

    [[nodiscard]]
    inline std::string component_display_name(const std::string_view identity) {
        if (identity == "itunes") {
            return "iTunes";
        }
        std::string name {identity};
        if (!name.empty() && name.front() >= 'a' && name.front() <= 'z') {
            name.front() = static_cast<char>(name.front() - 'a' + 'A');
        }
        return name;
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
        const std::filesystem::path& directory
    ) {
        return list::discover_ac_executables(directory);
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
        for (const auto& identity : identities) {
            rows.push_back({
                component_row_label(identity),
                identity + "_ac.exe",
                {"--settings"}
            });
        }
        return rows;
    }

} // namespace ac::main::settings
