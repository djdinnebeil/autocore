/**
 * \file logging_config.ixx
 * \brief Cached logger configuration shared by Auto Core processes.
 *
 * Settings are loaded from `config/logger.ini` and `config/components.list`
 * under the executable directory.
 * The first access caches the result for the process lifetime. A missing file
 * is written once from the portable defaults; invalid values keep those
 * defaults and do not rewrite an existing file.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.logging.config;

import std;

export namespace ac::logging::config {

    /**
     * \brief Returns whether central logging is enabled.
     *
     * True when `logger` is listed enabled (bare or `on`) in
     * `config/components.list`. A missing name, `off`, a duplicate entry,
     * or a list that cannot be read keeps this false. This flag does not
     * control console output; see `write_to_console()`.
     */
    [[nodiscard]] AC_API bool enabled();

    /**
     * \brief Returns whether component log messages are also written to stdout.
     *
     * Reads `[logger] write_to_console`. The default is off. Lowercase `on`
     * and `off` are the documented values. `true` is an alias for `on`, and
     * `false` is an alias for `off`. Any other value, or a missing key,
     * keeps that default.
     */
    [[nodiscard]] AC_API bool write_to_console();

    /**
     * \brief Returns the configured log directory.
     *
     * Reads `[logger] directory`. A missing or empty setting uses
     * `ac::paths::log_directory()`. Relative paths are resolved against the
     * executable directory. Dated `YYYY-MM-DD_main.log` files are written
     * here. Per-component files live under `components_directory()`.
     */
    [[nodiscard]] AC_API const std::filesystem::path& directory();

    /**
     * \brief Returns `<log directory>/components`.
     *
     * Each `ac::Component` writes `{date}_{name}.log` under
     * `components/<name>/`. Cached for the process lifetime.
     */
    [[nodiscard]] AC_API const std::filesystem::path& components_directory();

    /**
     * \brief Returns a startup report describing applied logger settings.
     *
     * The string is produced on first load and remains valid for the process
     * lifetime.
     */
    [[nodiscard]] AC_API std::string_view configuration_report();

} // namespace ac::logging::config
