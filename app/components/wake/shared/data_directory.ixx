/**
 * \file data_directory.ixx
 * \brief Resolved Wake operational-history directory.
 *
 * Reads `[wake] directory` from `config/wake.ini`. The compiled portable
 * default stays in `wake::defaults::directory`. Ordinary component logs
 * stay under the shared logging tree.
 */
export module wake_data_directory;

import std;
import auto_core.core.paths;
import wake_defaults;

export namespace wake {

    /**
     * \brief Returns the configured Wake history directory.
     *
     * A relative `[wake] directory` is resolved against the installation
     * root. An absolute value is used as-is. A missing file, missing key,
     * empty value, or unusable value keeps
     * `<installation root>/components/wake`. Cached for the process
     * lifetime. The directory is not created or validated.
     */
    [[nodiscard]]
    inline const std::filesystem::path& data_directory() {
        static const std::filesystem::path& directory =
            ac::paths::configured_directory(
                ac::paths::config_directory() / "wake.ini",
                "wake",
                "directory",
                ac::paths::installation_root() /
                    std::filesystem::path {
                        std::string {defaults::directory}
                    }
            );
        return directory;
    }

} // namespace wake
