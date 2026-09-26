/**
 * \file logging_config.ixx
 * \brief Cached logger configuration shared by Auto Core processes.
 *
 * Directory settings are loaded from `config/logger.ini` under the
 * installation root. A missing `logger.ini` keeps the defaults in memory
 * and does not write the file. The first access caches the result for the
 * process lifetime. A missing or unreadable `components.list` is reported
 * when the catalog is discovered from `*_ac.exe`; that report does not
 * write the file.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.logging.config;

import std;

export namespace ac::logging::config {

    /**
     * \brief Returns `[logger] merge_interval_seconds`.
     *
     * `0` disables periodic merging. `1` or greater is the interval in
     * seconds. The default is 60. A missing, negative, or malformed value
     * uses 60.
     */
    [[nodiscard]] AC_API std::uint64_t merge_interval_seconds();

    /**
     * \brief Returns whether `[logger] merge_logs_on_shutdown` is on.
     *
     * The default is on. Lowercase `on` and `off` are the documented
     * values. `true` is an alias for `on`, and `false` is an alias for
     * `off`. Any other value, or a missing key, keeps that default.
     * This is independent of `merge_interval_seconds()`.
     */
    [[nodiscard]] AC_API bool merge_logs_on_shutdown();

    /**
     * \brief Returns whether `[logger] write_logs_to_console` is on.
     *
     * The default is off. The same boolean forms as
     * `merge_logs_on_shutdown()` apply. When on, component log routes also
     * write to the console. Explicit console routes always write once.
     */
    [[nodiscard]] AC_API bool write_logs_to_console();

    /**
     * \brief Returns the configured log directory.
     *
     * Reads optional `[logger] directory`. A missing or empty setting uses
     * `ac::paths::log_directory()`. Relative paths are resolved against the
     * installation root. Per-component files live under
     * `components_directory()`.
     */
    [[nodiscard]] AC_API const std::filesystem::path& directory();

    /**
     * \brief Returns `<log directory>/components`.
     *
     * Each `ac::Component` writes `{date}_{name}.log` and
     * `{date}_{name}.main.log` under `components/<name>/`. Cached for the
     * process lifetime.
     */
    [[nodiscard]] AC_API const std::filesystem::path& components_directory();

    /**
     * \brief Returns whether `config/logger.ini` is absent.
     *
     * True only when the file does not exist. A present but unreadable
     * file stays false. The file is not created.
     */
    [[nodiscard]] AC_API bool ini_missing();

    /**
     * \brief Returns a startup report describing applied logger settings.
     *
     * The string is produced on first load and remains valid for the process
     * lifetime.
     */
    [[nodiscard]] AC_API std::string_view configuration_report();

} // namespace ac::logging::config
