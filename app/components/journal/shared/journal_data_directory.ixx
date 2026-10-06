/**
 * \file journal_data_directory.ixx
 * \brief Resolved Journal data directory.
 *
 * Reads `[journal] directory` from `config/journal.ini`. The compiled
 * portable default stays in `journal::defaults::directory`.
 */
export module journal_data_directory;

import std;
import auto_core.core.paths;

export namespace journal {

    /**
     * \brief Returns the configured Journal data directory.
     *
     * A relative `[journal] directory` is resolved against the installation
     * root. An absolute value is used as-is. A missing file, missing key,
     * empty value, or unusable value keeps
     * `<installation root>/components/journal`. Cached for the process
     * lifetime. The directory is not created or validated.
     */
    [[nodiscard]]
    inline const std::filesystem::path& data_directory() {
        static const std::filesystem::path& directory =
            ac::paths::configured_directory(
                ac::paths::config_directory() / "journal.ini",
                "journal",
                "directory",
                ac::paths::installation_root() / "components" / "journal"
            );
        return directory;
    }

} // namespace journal
