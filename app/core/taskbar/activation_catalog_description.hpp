/**
 * \file activation_catalog_description.hpp
 * \brief Keymap catalog text for taskbar activation commands.
 *
 * Header-only so iTunes can share the iTunes sentence without compiling
 * the taskbar module. A non-empty display name is the product name.
 * Otherwise a known application key supplies it. `activate_word`,
 * `activate_itunes`, and `activate_spotify` also note the setup
 * instructions shown when no application map registered that command.
 */
#pragma once

#include <string>
#include <string_view>

namespace ac::taskbar {

    namespace activation_detail {

        struct KnownActivation {
            std::string_view key;
            std::string_view product;
        };

        inline constexpr KnownActivation known_activations[] {
            {"adobe_acrobat", "Adobe Acrobat"},
            {"auto_core", "Auto Core"},
            {"chromium", "Chromium"},
            {"cursor", "Cursor"},
            {"discord", "Discord"},
            {"excel", "Microsoft Excel"},
            {"file_explorer", "File Explorer"},
            {"firefox", "Firefox"},
            {"firefox_private_browsing", "Firefox Private Browsing"},
            {"google_chrome", "Google Chrome"},
            {"icofx_the_professional_icon_editor", "IcoFX"},
            {"itunes", "iTunes"},
            {"kindle", "Kindle"},
            {"microsoft_edge", "Microsoft Edge"},
            {"microsoft_teams", "Microsoft Teams"},
            {"notepad", "Notepad"},
            {"notepad2", "Notepad"},
            {"onenote", "Microsoft OneNote"},
            {"powershell", "Windows PowerShell"},
            {"powershell_admin", "Windows PowerShell as administrator"},
            {"pycharm", "PyCharm"},
            {"spotify", "Spotify"},
            {"taskbar_builder", "Taskbar Builder"},
            {"task_manager", "Task Manager"},
            {"visual_studio", "Visual Studio"},
            {"visual_studio_code", "Visual Studio Code"},
            {"visual_studio_code_insiders", "Visual Studio Code Insiders"},
            {"word", "Microsoft Word"},
            {"wordpad", "WordPad"},
            {"wsl2", "WSL"},
            {"zoom_workplace", "Zoom Workplace"},
        };

        inline constexpr std::string_view numbered_auto_core_prefix =
            "activate_auto_core_";
        inline constexpr std::string_view setup_instructions_clause =
            ". Shows setup instructions when no application map is available.";

        [[nodiscard]]
        constexpr std::string_view trim_catalog_text(
            const std::string_view value
        ) noexcept {
            const auto first = value.find_first_not_of(" \t");
            if (first == std::string_view::npos) {
                return {};
            }
            const auto last = value.find_last_not_of(" \t");
            return value.substr(first, last - first + 1);
        }

        [[nodiscard]]
        constexpr bool is_numbered_auto_core_activation(
            const std::string_view command
        ) noexcept {
            if (!command.starts_with(numbered_auto_core_prefix)) {
                return false;
            }
            const std::string_view suffix =
                command.substr(numbered_auto_core_prefix.size());
            if (suffix.empty()) {
                return false;
            }
            for (const char character : suffix) {
                if (character < '0' || character > '9') {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]]
        constexpr bool shows_setup_instructions(
            const std::string_view command
        ) noexcept {
            return command == "activate_word" ||
                command == "activate_itunes" ||
                command == "activate_spotify";
        }

        [[nodiscard]]
        constexpr const KnownActivation* find_known_activation(
            const std::string_view key
        ) noexcept {
            for (const KnownActivation& entry : known_activations) {
                if (entry.key == key) {
                    return &entry;
                }
            }
            return nullptr;
        }

    } // namespace activation_detail

    /**
     * \brief Catalog description for one activation command.
     *
     * `activate_auto_core_<digits>` is "Activates another Auto Core or Auto
     * Core Component". `activate_auto_core` and nonnumeric suffixes do not
     * use that sentence. Any other `activate_*` command uses `display_name`
     * when it is non-empty, then the known product name for `application`.
     * A missing product leaves the command undescribed.
     */
    [[nodiscard]]
    inline std::string activation_catalog_description(
        const std::string_view command,
        const std::string_view application,
        const std::string_view display_name
    ) {
        using namespace activation_detail;
        if (is_numbered_auto_core_activation(command)) {
            return "Activates another Auto Core or Auto Core Component";
        }
        if (!command.starts_with("activate_")) {
            return {};
        }

        const std::string_view trimmed_display = trim_catalog_text(display_name);
        const KnownActivation* known = find_known_activation(application);
        const std::string_view product = !trimmed_display.empty()
            ? trimmed_display
            : (known != nullptr ? known->product : std::string_view {});
        if (product.empty()) {
            return {};
        }

        std::string description = "Activates ";
        description.append(product);
        if (shows_setup_instructions(command)) {
            description.append(setup_instructions_clause);
        }
        return description;
    }

} // namespace ac::taskbar
