/**
 * \file logging_config.ixx
 * \brief Cached logging policy shared by Auto Core processes.
 *
 * Shared file and console policy is loaded from `config/logging_policy.ini`.
 * The file is cached on first use for the process lifetime and is not
 * written here. `resolve()` loads the shared policy at a caller-chosen
 * point. A missing `components.list` is noted in the startup report.
 * This module does not discover executables.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.logging.config;

import std;

export namespace ac::logging::config {

    /**
     * \brief Minimum diagnostic severity from `[logging] level`.
     *
     * `trace` admits trace, debug, info, alert.
     * `debug` admits debug, info, and alert.
     * `info` admits info, alert.
     *  `alert` admits alert.
     */
    enum class LogLevel : unsigned char {
        trace = 0,
        debug = 1,
        info = 2,
        alert = 3
    };

    /**
     * \brief Fallback used when a family's `logging` value is absent
     * or invalid.
     *
     * `global_default` uses `component_logging_default`, then compiled on.
     * `off` does not consult `component_logging_default`.
     */
    enum class LoggingFallback {
        global_default,
        off
    };

    /**
     * \brief Explicit family logging scope.
     *
     * `name` selects section `[name]`. Scope `auto_core` reads
     * `config/auto_core.ini`. Every other scope reads
     * `config/components/<name>.ini`.
     * `fallback` is data. The resolver does not infer it from `name`.
     */
    struct LoggingScope {
        std::string_view name;
        LoggingFallback fallback = LoggingFallback::global_default;
    };

    /**
     * \brief Loads `config/logging_policy.ini` into the process cache.
     *
     * The first call reads the file. Later calls reuse that result.
     * Call this after first-run handling and before normal startup logs.
     */
    AC_API void resolve();

    /**
     * \brief Returns whether `[logging] disable_all` is on.
     *
     * The default is off. When on, diagnostic output is suppressed,
     * the file copy of `print` / `printnl` is suppressed, and
     * component-family logging is not resolved. `print` console
     * output remains available.
     */
    [[nodiscard]] AC_API bool disable_all();

    /**
     * \brief Returns whether `[logging] experimental` is on.
     *
     * The default is on. This temporary setting controls the cleanup
     * utility's startup report channel.
     */
    [[nodiscard]] AC_API bool experimental();

    /**
     * \brief Returns whether `[logging] log_console_output` is on.
     *
     * The default is on. When on, `print` / `printnl` output participates
     * in the component file sink. It does not control the console write.
     * The file copy also requires `disable_all` off, family logging on,
     * and `write_logs_to_files` on.
     */
    [[nodiscard]] AC_API bool log_console_output();

    /**
     * \brief Returns whether `[logging] write_logs_to_files` is on.
     *
     * The default is on. This is the master file-output gate for component
     * logging, including eligible `print` / `printnl` copies. It applies
     * only after `disable_all` is off and family logging is on.
     */
    [[nodiscard]] AC_API bool write_logs_to_files();

    /**
     * \brief Returns whether `[logging] write_logs_to_console` is on.
     *
     * The default is off. When on, enabled diagnostic logs are mirrored
     * to the console. `print` is unaffected.
     */
    [[nodiscard]] AC_API bool write_logs_to_console();

    /**
     * \brief Returns `[logging] level`.
     *
     * The default is `debug`. Missing or invalid values use `debug`.
     */
    [[nodiscard]] AC_API LogLevel level();

    /**
     * \brief Returns whether `[logging] write_alerts_to_console` is on.
     *
     * The default is on. Enabled alerts are shown on the console when
     * this is on. A message already mirrored by `write_logs_to_console`
     * is written once. This does not copy `print` output into the log.
     */
    [[nodiscard]] AC_API bool write_alerts_to_console();

    /**
     * \brief Returns `[logging] component_logging_default`.
     *
     * The default is on. This is the normal and unscoped fallback. A
     * scope whose fallback is `off` does not use it.
     */
    [[nodiscard]] AC_API bool component_logging_default();

    /**
     * \brief Returns whether logging is on for an explicit family scope.
     *
     * Reads section `[scope.name]` key `logging` from `config/auto_core.ini`
     * when the scope is `auto_core`, otherwise from
     * `config/components/<scope.name>.ini`. The file is read
     * at most once per scope for the process lifetime. Does not write the
     * file and does not report through `ac::Component`. When `disable_all`
     * is on, returns false without reading the component INI.
     */
    [[nodiscard]] AC_API bool component_logging_enabled(const LoggingScope& scope);

    /**
     * \brief Returns the shared log directory from `logging_policy.ini`.
     *
     * Reads optional `[logging] directory`. A missing or empty setting uses
     * `ac::paths::log_directory()`. Relative paths are resolved against the
     * installation root. This is not a `logger.ini` setting. Per-component
     * files and merged output both use this directory.
     */
    [[nodiscard]] AC_API const std::filesystem::path& directory();

    /**
     * \brief Returns `[logging] max_directory_mb`.
     *
     * The default is 100. Missing, partial, or negative values use 100.
     * `0` is stored as written and does not disable logging.
     */
    [[nodiscard]] AC_API std::int64_t max_directory_mb();

    /**
     * \brief Returns `[logging] retention_days`.
     *
     * The default is 30. Missing, partial, or negative values use 30.
     * `0` is stored as written and does not disable logging.
     */
    [[nodiscard]] AC_API std::int64_t retention_days();

    /**
     * \brief Returns `<log directory>/components`.
     *
     * Each `ac::Component` writes `{date}_{name}.log` under
     * `components/<name>/` when file logging is enabled. Cached for the
     * process lifetime.
     */
    [[nodiscard]] AC_API const std::filesystem::path& components_directory();

    /**
     * \brief Returns whether `config/logging_policy.ini` is absent.
     *
     * True only when the file does not exist. A present but unreadable
     * file stays false. The file is not created.
     */
    [[nodiscard]] AC_API bool logging_ini_missing();

    /**
     * \brief Returns diagnostics from resolving `logging_policy.ini`.
     *
     * Successful resolution returns an empty string. The string is produced
     * on first load and remains valid for the process lifetime.
     */
    [[nodiscard]] AC_API std::string_view configuration_report();

} // namespace ac::logging::config
