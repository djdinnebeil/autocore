/**
 * \file logging_config_detail.hpp
 * \brief Pure resolution of shared logging settings.
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace ac::logging::config::detail {

    enum class LogLevel : unsigned char {
        trace = 0,
        debug = 1,
        info = 2,
        alert = 3
    };

    enum class LoggingFallback {
        global_default,
        off
    };

    struct LoggingRaw {
        std::optional<std::string_view> disable_all;
        std::optional<std::string_view> level;
        std::optional<std::string_view> log_console_output;
        std::optional<std::string_view> write_logs_to_files;
        std::optional<std::string_view> write_logs_to_console;
        std::optional<std::string_view> write_alerts_to_console;
        std::optional<std::string_view> component_logging_default;
        std::optional<std::string_view> max_directory_mb;
        std::optional<std::string_view> retention_days;
        std::optional<std::filesystem::path> directory;
        std::optional<std::string_view> experimental;
    };

    struct LoggingSettings {
        bool disable_all;
        LogLevel level;
        bool log_console_output;
        bool write_logs_to_files;
        bool write_logs_to_console;
        bool write_alerts_to_console;
        bool component_logging_default;
        std::int64_t max_directory_mb;
        std::int64_t retention_days;
        std::filesystem::path directory;
        std::filesystem::path components_directory;
        std::string report;
        bool experimental;
    };

    /**
     * \brief Resolves `logging_policy.ini` keys.
     *
     * `disable_all` defaults to off. `level` defaults to debug.
     * `log_console_output` defaults  to on. `write_logs_to_files` defaults to on.
     * `write_logs_to_console` defaults to off.
     * `write_alerts_to_console` defaults to on.
     * `component_logging_default` defaults to on.
     * `max_directory_mb` defaults to 100. `retention_days` defaults to 30.
     * `experimental` defaults to off.
     * `0` is a stored purge value, not a disabled safeguard.
     * A relative directory
     * resolves against `installation_root`. An empty directory uses
     * `default_directory`. A leftover `enabled` key is ignored.
     */
    LoggingSettings resolve_logging(
        const LoggingRaw& raw,
        const std::filesystem::path& default_directory,
        const std::filesystem::path& installation_root
    );

    /**
     * \brief Resolves one family's `logging` value.
     *
     * A valid explicit value wins. Otherwise `LoggingFallback::off` is off
     * and does not consult `component_logging_default`.
     * `LoggingFallback::global_default` uses that flag, which is already
     * the compiled `on` when the global key was missing or invalid.
     */
    [[nodiscard]]
    bool resolve_component_logging(
        LoggingFallback fallback,
        const std::optional<std::string_view>& logging,
        bool component_logging_default
    ) noexcept;

} // namespace ac::logging::config::detail
