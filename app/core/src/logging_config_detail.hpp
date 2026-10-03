/**
 * \file logging_config_detail.hpp
 * \brief Pure resolution of shared logging and logger-merge settings.
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace ac::logging::config::detail {

    enum class LogPrintMode {
        log,
        print
    };

    enum class LoggingFallback {
        global_default,
        off
    };

    struct LoggingRaw {
        std::optional<std::string_view> disable_all;
        std::optional<std::string_view> write_logs_to_files;
        std::optional<std::string_view> write_logs_to_console;
        std::optional<std::string_view> log_print_mode;
        std::optional<std::string_view> component_logging_default;
        std::optional<std::filesystem::path> directory;
    };

    struct LoggingSettings {
        bool disable_all;
        bool write_logs_to_files;
        bool write_logs_to_console;
        LogPrintMode log_print_mode;
        bool component_logging_default;
        std::filesystem::path directory;
        std::filesystem::path components_directory;
        std::string report;
    };

    struct LoggerRaw {
        std::optional<std::string_view> merge_interval_seconds;
        std::optional<std::string_view> merge_logs_on_shutdown;
    };

    struct LoggerSettings {
        std::uint64_t merge_interval_seconds;
        bool merge_logs_on_shutdown;
        std::string report;
    };

    /**
     * \brief Resolves `logging.ini` keys.
     *
     * `disable_all` defaults to off. `write_logs_to_files` defaults to on.
     * `write_logs_to_console` defaults to off. `log_print_mode` defaults to
     * print. `component_logging_default` defaults to on. A relative
     * directory resolves against `installation_root`. An empty directory
     * uses `default_directory`. A leftover `enabled` key is ignored.
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

    /**
     * \brief Resolves `logger.ini` merge keys.
     *
     * Interval `0` is kept. A missing or invalid interval uses 60.
     * Shutdown merge defaults to on.
     */
    LoggerSettings resolve_logger(const LoggerRaw& raw);

} // namespace ac::logging::config::detail
