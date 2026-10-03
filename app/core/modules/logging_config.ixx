/**
 * \file logging_config.ixx
 * \brief Cached logging policy shared by Auto Core processes.
 *
 * Shared file and console policy is loaded from `config/logging.ini`.
 * Merger timing is loaded from `config/logger.ini`. Each file is cached
 * on first use for the process lifetime and is not written here.
 * `resolve()` loads the shared policy at a caller-chosen point.
 * A missing or unreadable `components.list` is reported when the catalog
 * is discovered from `*_ac.exe`; that report does not write the file.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.logging.config;

import std;

export namespace ac::logging::config {

    /**
     * \brief How `log_print` is classified before routing.
     *
     * `log` is logging-controlled. `print` is user-facing.
     */
    enum class LogPrintMode {
        log,
        print
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
     * `name` selects `config/<name>.ini` and section `[name]`.
     * `fallback` is data. The resolver does not infer it from `name`.
     */
    struct LoggingScope {
        std::string_view name;
        LoggingFallback fallback = LoggingFallback::global_default;
    };

    /**
     * \brief Loads `config/logging.ini` into the process cache.
     *
     * The first call reads the file. Later calls reuse that result.
     * Call this after first-run handling and before normal startup logs.
     */
    AC_API void resolve();

    /**
     * \brief Returns whether `[logging] disable_all` is on.
     *
     * The default is off. When on, logging-controlled output is suppressed
     * and component-family logging is not resolved. User-facing console
     * output remains available.
     */
    [[nodiscard]] AC_API bool disable_all();

    /**
     * \brief Returns whether `[logging] write_logs_to_files` is on.
     *
     * The default is on. This is the global file sink. It applies only
     * after `disable_all` is off and family logging is on.
     */
    [[nodiscard]] AC_API bool write_logs_to_files();

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
     * \brief Returns whether `[logging] write_logs_to_console` is on.
     *
     * The default is off. When off, logging-controlled output does not
     * go to the console. User-facing calls are unaffected.
     */
    [[nodiscard]] AC_API bool write_logs_to_console();

    /**
     * \brief Returns `[logging] log_print_mode`.
     *
     * The default is `print`. Missing or invalid values use `print`.
     */
    [[nodiscard]] AC_API LogPrintMode log_print_mode();

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
     * Reads `config/<scope.name>.ini` section `[scope.name]` key `logging`
     * at most once per scope for the process lifetime. Does not write the
     * file and does not report through `ac::Component`. When `disable_all`
     * is on, returns false without reading the component INI.
     */
    [[nodiscard]] AC_API bool component_logging_enabled(const LoggingScope& scope);

    /**
     * \brief Returns the shared log directory from `logging.ini`.
     *
     * Reads optional `[logging] directory`. A missing or empty setting uses
     * `ac::paths::log_directory()`. Relative paths are resolved against the
     * installation root. This is not a `logger.ini` setting. Per-component
     * files and merged output both use this directory.
     */
    [[nodiscard]] AC_API const std::filesystem::path& directory();

    /**
     * \brief Returns `<log directory>/components`.
     *
     * Each `ac::Component` writes `{date}_{name}.log` and
     * `{date}_{name}.main.log` under `components/<name>/` when file
     * logging is enabled. Cached for the process lifetime.
     */
    [[nodiscard]] AC_API const std::filesystem::path& components_directory();

    /**
     * \brief Returns whether `config/logging.ini` is absent.
     *
     * True only when the file does not exist. A present but unreadable
     * file stays false. The file is not created.
     */
    [[nodiscard]] AC_API bool logging_ini_missing();

    /**
     * \brief Returns whether `config/logger.ini` is absent.
     *
     * True only when the file does not exist. A present but unreadable
     * file stays false. The file is not created.
     */
    [[nodiscard]] AC_API bool logger_ini_missing();

    /**
     * \brief Returns a startup report describing applied `logging.ini` settings.
     *
     * The string is produced on first load and remains valid for the process
     * lifetime.
     */
    [[nodiscard]] AC_API std::string_view configuration_report();

    /**
     * \brief Returns a startup report describing applied `logger.ini` settings.
     *
     * The string is produced on first merger-policy load and remains valid
     * for the process lifetime.
     */
    [[nodiscard]] AC_API std::string_view logger_configuration_report();

} // namespace ac::logging::config
