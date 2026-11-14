/**
 * \file installation_layout.hpp
 * \brief Pure derivation of executable directories from an image path.
 *
 * Unit tests pass constructed paths. Runtime uses GetModuleFileNameW.
 * The process current working directory is never used.
 */
#pragma once

#include <filesystem>
#include <string_view>

namespace ac::paths::detail {

    struct InstallationLayout {
        std::filesystem::path bin_directory;
        std::filesystem::path installation_root;
        std::filesystem::path app_directory;
        std::filesystem::path component_executable_directory;
    };

    [[nodiscard]]
    inline bool directory_name_is(
        const std::filesystem::path& directory,
        const std::string_view expected
    ) {
        const auto name = directory.filename().string();
        if (name.size() != expected.size()) {
            return false;
        }
        for (std::size_t index = 0; index < name.size(); ++index) {
            const auto left = name[index];
            const auto right = expected[index];
            const auto lower_left = (left >= 'A' && left <= 'Z')
                ? static_cast<char>(left - 'A' + 'a')
                : left;
            const auto lower_right = (right >= 'A' && right <= 'Z')
                ? static_cast<char>(right - 'A' + 'a')
                : right;
            if (lower_left != lower_right) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]]
    inline InstallationLayout layout_from_image_path(
        const std::filesystem::path& image_path
    ) {
        InstallationLayout layout;
        layout.bin_directory = image_path.parent_path();
        layout.installation_root = layout.bin_directory.parent_path();
        const bool in_components = directory_name_is(
            layout.bin_directory,
            "components"
        );
        layout.app_directory = in_components
            ? layout.installation_root / "app"
            : layout.bin_directory;
        layout.component_executable_directory = in_components
            ? layout.bin_directory
            : layout.installation_root / "components";
        return layout;
    }

} // namespace ac::paths::detail
