/**
 * \file spotify_config_detail.hpp
 * \brief Pure resolution of the Spotify data directory.
 */
#pragma once

#include <filesystem>
#include <optional>

namespace ac::spotify::config::detail {

    struct RawSettings {
        std::optional<std::filesystem::path> directory;
    };

    std::filesystem::path resolve(
        const RawSettings& raw,
        const std::filesystem::path& default_directory,
        const std::filesystem::path& installation_root
    );

} // namespace ac::spotify::config::detail
