/**
 * \file core_config.ixx
 * \brief Cached configuration shared by Auto Core processes.
 *
 * Settings are loaded from `config/auto_core.ini` under the executable
 * directory. The first access caches the result for the process lifetime. A
 * missing file is written once from the portable defaults. Missing or invalid
 * values use those same defaults and do not rewrite an existing file.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.config;

import std;

export namespace ac::config {

    /**
     * \brief Values from `[auto_core]` after initialization.
     *
     * `warn_without_winkey_mapping` defaults to true. Only lowercase `false`
     * turns the warning off. Any other value keeps the default.
     */
    struct CoreSettings {
        bool warn_without_winkey_mapping;
    };

    /**
     * \brief Writes missing `config/` files from portable defaults.
     *
     * Creates `config/` if needed. Existing live files are never overwritten.
     * Does not read repo `defaults/`.
     */
    AC_API void seed_missing_config_files();

    /**
     * \brief Writes `journal/journal_choices.ini` once if it is missing.
     *
     * Creates the journal data directory if needed. Existing live files are
     * never overwritten. Does not read repo `defaults/`.
     */
    AC_API void seed_missing_journal_choices();

    /**
     * \brief Writes `config/journal.ini` once if it is missing.
     *
     * \param directory `[journal] directory` to store. Empty uses `journal`.
     * \return `false` when the config directory or file cannot be written.
     */
    AC_API bool write_journal_ini_if_missing(std::string_view directory);

    /**
     * \brief Writes `config/taskbar.ini` once if it is missing.
     *
     * \param directory `[taskbar] directory` to store. Empty uses `taskbar`.
     * \return `false` when the config directory or file cannot be written.
     */
    AC_API bool write_taskbar_ini_if_missing(std::string_view directory);

    /**
     * \brief Writes `config/writer.ini` once if it is missing.
     *
     * \param directory `[writer] directory` to store. Empty uses `writer`.
     * \param notes_directory `[writer] notes_directory` to store. Empty uses
     *        `notes`.
     * \return `false` when the config directory or file cannot be written.
     */
    AC_API bool write_writer_ini_if_missing(
        std::string_view directory,
        std::string_view notes_directory
    );

    /**
     * \brief Writes `config/server.ini` once if it is missing.
     *
     * \param document_root `[server] document_root` to store. Empty uses
     *        `server`.
     * \param port `[server] port` to store. Values outside `1`–`65535` fail.
     * \return `false` when the port is invalid or the config directory or
     *         file cannot be written.
     */
    AC_API bool write_server_ini_if_missing(
        std::string_view document_root,
        int port
    );

    /**
     * \brief Rewrites `[server] port` in the live `config/server.ini`.
     *
     * Preserves a non-empty stored `document_root`; otherwise stores
     * `server`. Creates the file with both keys when it is missing.
     *
     * \param port `[server] port` to store. Values outside `1`–`65535` fail.
     * \return `false` when the port is invalid or the file cannot be
     *         written.
     */
    AC_API bool write_server_ini_port(int port);

    /**
     * \brief Rewrites `[server] document_root` in the live
     *        `config/server.ini`.
     *
     * Preserves a valid stored `port`; otherwise stores `8585`. Empty
     * `document_root` stores `server`. Creates the file with both keys when
     * it is missing.
     *
     * \return `false` when the file cannot be written.
     */
    AC_API bool write_server_ini_document_root(std::string_view document_root);

    /**
     * \brief Loads `config/auto_core.ini` once in this process.
     *
     * Seeds missing config files first. Always succeeds. Call before
     * `core_settings()`.
     */
    AC_API void initialize_core_settings();

    /**
     * \brief Returns the cached settings after initialization.
     *
     * \return A process-lifetime reference to the loaded settings.
     *
     * Calls `std::terminate()` if initialization was skipped.
     */
    [[nodiscard]] AC_API const CoreSettings& core_settings() noexcept;

} // namespace ac::config
