/**
 * \file core_config.ixx
 * \brief Cached configuration shared by Auto Core processes.
 *
 * Settings are loaded from `config/auto_core.ini` under the executable
 * directory. The first access caches the result for the process lifetime.
 * A missing or malformed file keeps the defaults in memory and does not
 * write the file. Only `auto_core_config.exe` creates or rewrites
 * `auto_core.ini`. File existence is the initialization marker.
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
     * \brief Writes `journal/journal_choices.ini` once if it is missing.
     *
     * Creates the journal data directory if needed. Existing live files are
     * never overwritten. Does not read repo `defaults/`. Called only from
     * `journal_config.exe`.
     */
    AC_API void seed_missing_journal_choices();

    /**
     * \brief Loads `config/auto_core.ini` once in this process.
     *
     * Always succeeds. Missing or malformed values keep defaults. Call before
     * `core_settings()`.
     */
    AC_API void initialize_core_settings();

    /**
     * \brief Returns a process-lifetime report for `auto_core.ini` load issues.
     *
     * Empty when the file loaded with a valid `[auto_core]` warning value.
     * Call after
     * `initialize_core_settings()`.
     */
    [[nodiscard]] AC_API std::string_view core_settings_report() noexcept;

    /**
     * \brief Returns the cached settings after initialization.
     *
     * \return A process-lifetime reference to the loaded settings.
     *
     * Calls `std::terminate()` if initialization was skipped.
     */
    [[nodiscard]] AC_API const CoreSettings& core_settings() noexcept;

} // namespace ac::config
