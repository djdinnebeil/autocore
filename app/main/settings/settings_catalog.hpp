/**
 * \file settings_catalog.hpp
 * \brief Discovery and menu rows for `auto_core_settings.exe`.
 *
 * Installed `*_settings.exe` files are the Settings catalog. This scan does
 * not read `components.list` and does not require `*_ac.exe`.
 */
#pragma once

#include "../../core/component/components_list_detail.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ac::main::settings {

    namespace list = ac::config::components_list;

    enum class StartupStep {
        show_menu,
        launch_init
    };

    enum class InitRecheck {
        show_menu,
        exit_uninitialized
    };

    struct MenuRow {
        std::string label;
        std::string executable;
    };

    [[nodiscard]]
    inline StartupStep startup_step(const bool sentinel_exists) noexcept {
        return sentinel_exists
            ? StartupStep::show_menu
            : StartupStep::launch_init;
    }

    [[nodiscard]]
    inline InitRecheck recheck_after_init(const bool sentinel_exists) noexcept {
        return sentinel_exists
            ? InitRecheck::show_menu
            : InitRecheck::exit_uninitialized;
    }

    [[nodiscard]]
    inline std::string component_row_label(const std::string_view identity) {
        std::string label = "Modify ";
        label.append(identity);
        label.append(" settings");
        return label;
    }

    [[nodiscard]]
    inline bool equals_ignore_case(
        const std::string_view text,
        const std::string_view expected
    ) noexcept {
        if (text.size() != expected.size()) {
            return false;
        }
        for (std::size_t index = 0; index < text.size(); ++index) {
            char character = text[index];
            if (character >= 'A' && character <= 'Z') {
                character = static_cast<char>(character - 'A' + 'a');
            }
            if (character != expected[index]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]]
    inline bool has_settings_exe_suffix(const std::string_view filename) noexcept {
        constexpr std::string_view suffix = "_settings.exe";
        if (filename.size() <= suffix.size()) {
            return false;
        }
        return equals_ignore_case(
            filename.substr(filename.size() - suffix.size()),
            suffix
        );
    }

    [[nodiscard]]
    inline std::vector<std::string> discover_settings_executables(
        const std::filesystem::path& directory
    ) {
        std::vector<std::string> names;
        std::error_code error;
        const auto iterator = std::filesystem::directory_iterator(directory, error);
        if (error) {
            return names;
        }

        constexpr std::string_view suffix = "_settings.exe";
        constexpr std::string_view excluded = "auto_core_settings.exe";
        for (const auto& entry : iterator) {
            std::error_code status_error;
            if (!entry.is_regular_file(status_error) || status_error) {
                continue;
            }

            const auto u8_name = entry.path().filename().u8string();
            const std::string filename(u8_name.begin(), u8_name.end());
            if (!has_settings_exe_suffix(filename) ||
                equals_ignore_case(filename, excluded)) {
                continue;
            }

            const auto name = filename.substr(0, filename.size() - suffix.size());
            if (!list::is_valid_component_name(name)) {
                continue;
            }
            if (std::ranges::find(names, name) == names.end()) {
                names.emplace_back(name);
            }
        }

        std::ranges::sort(names);
        return names;
    }

    [[nodiscard]]
    inline std::vector<MenuRow> menu_rows(
        const std::vector<std::string>& identities
    ) {
        std::vector<MenuRow> rows;
        rows.push_back({"Modify Auto Core settings", "auto_core_config.exe"});
        for (const auto& identity : identities) {
            rows.push_back({
                component_row_label(identity),
                identity + "_settings.exe"
            });
        }
        return rows;
    }

} // namespace ac::main::settings
