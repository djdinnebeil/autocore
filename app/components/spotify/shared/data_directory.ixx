/**
 * \file data_directory.ixx
 * \brief Resolved Spotify data directory.
 *
 * Reads `[spotify] directory` from `config/spotify.ini`. The compiled
 * portable default stays in `spotify::defaults::directory`.
 */
export module spotify_data_directory;

import std;
import auto_core.core.paths;
import spotify_defaults;

export namespace spotify {

    /**
     * \brief Returns the configured Spotify data directory.
     *
     * A relative `[spotify] directory` is resolved against the installation
     * root. An absolute value is used as-is. A missing file, missing key,
     * empty value, or unusable value keeps
     * `<installation root>/components/spotify`. Cached for the process
     * lifetime. The directory is not created or validated.
     */
    [[nodiscard]]
    inline const std::filesystem::path& data_directory() {
        static const std::filesystem::path& directory =
            ac::paths::configured_directory(
                ac::paths::config_directory() / "spotify.ini",
                "spotify",
                "directory",
                ac::paths::installation_root() /
                    std::filesystem::path {
                        std::string {defaults::directory}
                    }
            );
        return directory;
    }

} // namespace spotify
