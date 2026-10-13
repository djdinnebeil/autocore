/**
 * \file core_config.ixx
 * \brief Cached warning configuration shared by Auto Core processes.
 *
 * Settings are loaded from `config/warnings.ini`. The first access caches
 * the result for the lifetime of the process. A missing or malformed file
 * keeps the defaults in memory and does not write the file. Only
 * `warnings_config.exe` creates or rewrites `warnings.ini`.
 * `config/auto_core.ini` remains the initialization sentinel and is not
 * loaded here. `[auto_core] logging` is family logging for `auto_core.exe`.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.config;

import std;

export namespace ac::config {

    /**
     * \brief Values from `[warnings]` after initialization.
     *
     * Both warnings default to on. Only lowercase `off` turns a warning
     * off. Any other value keeps that warning on.
     */
    struct CoreSettings {
        bool missing_winkey_mapping;
        bool unmapped_key;
    };

    /**
     * \brief Loads `config/warnings.ini` once in this process.
     *
     * Always succeeds. Missing or malformed values keep defaults. Call before
     * `core_settings()`.
     */
    AC_API void initialize_core_settings();

    /**
     * \brief Returns a process-lifetime report for `warnings.ini` load issues.
     *
     * Empty when the file loaded with valid `[warnings]` values.
     * Call after `initialize_core_settings()`.
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
