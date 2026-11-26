/**
 * \file activation_catalog_description.hpp
 * \brief Keymap catalog text for taskbar activation commands.
 *
 * Header-only so iTunes can share the iTunes sentence without compiling
 * the taskbar module. A non-empty display name is the product name.
 * Otherwise a known application key supplies it.
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
            {"chrome", "Google Chrome"},
            {"chromium", "Chromium"},
            {"cursor", "Cursor"},
            {"discord", "Discord"},
            {"excel", "Microsoft Excel"},
            {"file_explorer", "File Explorer"},
            {"firefox", "Firefox"},
            {"firefox_private_browsing", "Firefox Private Browsing"},
            {"icofx_the_professional_icon_editor", "IcoFX"},
            {"itunes", "iTunes"},
            {"kindle", "Kindle"},
            {"microsoft_edge", "Microsoft Edge"},
            {"microsoft_teams", "Microsoft Teams"},
            {"notepad", "Notepad"},
            {"notepad2", "Notepad Classic"},
            {"onenote", "Microsoft OneNote"},
            {"powershell", "Windows PowerShell"},
            {"pycharm", "PyCharm"},
            {"spotify", "Spotify"},
            {"taskbar_builder", "Taskbar Builder"},
            {"task_manager", "Task Manager"},
            {"visual_studio", "Visual Studio"},
            {"visual_studio_code", "Visual Studio Code"},
            {"visual_studio_code_insiders", "Visual Studio Code Insiders"},
            {"word", "Microsoft Word"},
            {"wsl2", "WSL"},
            {"zoom_workplace", "Zoom Workplace"},
        };

        inline constexpr std::string_view numbered_auto_core_prefix =
            "activate_auto_core_";

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
        constexpr bool is_numbered_suffix(
            const std::string_view suffix
        ) noexcept {
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
        constexpr bool is_numbered_auto_core_activation(
            const std::string_view command
        ) noexcept {
            constexpr std::string_view taskbar_prefix = "taskbar_auto_core_";
            if (command.starts_with(numbered_auto_core_prefix)) {
                return is_numbered_suffix(
                    command.substr(numbered_auto_core_prefix.size())
                );
            }
            if (command.starts_with(taskbar_prefix)) {
                return is_numbered_suffix(command.substr(taskbar_prefix.size()));
            }
            return false;
        }

        [[nodiscard]]
        constexpr bool is_generated_taskbar_command(
            const std::string_view command,
            const std::string_view application
        ) noexcept {
            constexpr std::string_view prefix = "taskbar_";
            return command.starts_with(prefix) &&
                command.substr(prefix.size()) == application;
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
     * `activate_auto_core_<digits>` and `taskbar_auto_core_<digits>` are
     * "Activates another Auto Core or Auto Core Component."
     * `activate_auto_core` and nonnumeric suffixes do not use that sentence.
     * Any other `activate_*` command, and a generated `taskbar_<application>`
     * command, uses `display_name` when it is non-empty, then the known
     * product name for `application`. A missing product leaves the command
     * undescribed. The sentence ends with a period.
     */
    [[nodiscard]]
    inline std::string activation_catalog_description(
        const std::string_view command,
        const std::string_view application,
        const std::string_view display_name
    ) {
        using namespace activation_detail;
        if (is_numbered_auto_core_activation(command)) {
            return "Activates another Auto Core or Auto Core Component.";
        }
        if (!command.starts_with("activate_") &&
            !is_generated_taskbar_command(command, application)) {
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
        if (description.back() != '.') {
            description.push_back('.');
        }
        return description;
    }

} // namespace ac::taskbar
