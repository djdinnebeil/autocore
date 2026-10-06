/**
 * \file installation_layout.hpp
 * \brief Pure derivation of bin_directory and installation_root from an
 *        executable image path.
 *
 * Unit tests pass constructed paths. Runtime uses GetModuleFileNameW.
 * The process current working directory is never used.
 */
#pragma once

#include <filesystem>

namespace ac::paths::detail {

    struct InstallationLayout {
        std::filesystem::path bin_directory;
        std::filesystem::path installation_root;
    };

    [[nodiscard]]
    inline InstallationLayout layout_from_image_path(
        const std::filesystem::path& image_path
    ) {
        InstallationLayout layout;
        layout.bin_directory = image_path.parent_path();
        layout.installation_root = layout.bin_directory.parent_path();
        return layout;
    }

} // namespace ac::paths::detail
