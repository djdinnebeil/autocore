#include "configured_directory.hpp"
#include "logging_config_detail.hpp"

#include <charconv>
#include <cstdint>

namespace ac::logging::config::detail {

    namespace {
        std::optional<bool> parse_bool(
            const std::optional<std::string_view> value
        ) noexcept {
            if (value == "on") {
                return true;
            }
            if (value == "off") {
                return false;
            }
            return std::nullopt;
        }

        std::optional<LogLevel> parse_level(
            const std::optional<std::string_view> value
        ) noexcept {
            if (value == "trace") {
                return LogLevel::trace;
            }
            if (value == "debug") {
                return LogLevel::debug;
            }
            if (value == "info") {
                return LogLevel::info;
            }
            if (value == "alert") {
                return LogLevel::alert;
            }
            return std::nullopt;
        }

        std::optional<std::int64_t> parse_non_negative(
            const std::optional<std::string_view> value
        ) noexcept {
            if (!value) {
                return std::nullopt;
            }
            std::int64_t parsed = 0;
            const auto result = std::from_chars(
                value->data(),
                value->data() + value->size(),
                parsed
            );
            if (result.ec != std::errc {} ||
                result.ptr != value->data() + value->size() ||
                parsed < 0) {
                return std::nullopt;
            }
            return parsed;
        }

        void append_integer(
            std::string& report,
            const std::string_view key,
            const bool used_default,
            const std::int64_t default_value
        ) {
            if (!used_default) {
                return;
            }
            report += key;
            report += " missing or invalid; using ";
            report += std::to_string(default_value);
            report += '\n';
        }

        void append_bool(
            std::string& report,
            const std::string_view key,
            const bool used_default,
            const std::string_view default_word
        ) {
            if (!used_default) {
                return;
            }
            report += key;
            report += " missing or invalid; using ";
            report += default_word;
            report += '\n';
        }
    }

    LoggingSettings resolve_logging(
        const LoggingRaw& raw,
        const std::filesystem::path& default_directory,
        const std::filesystem::path& installation_root
    ) {
        LoggingSettings settings {
            .disable_all = false,
            .level = LogLevel::debug,
            .log_console_output = true,
            .write_logs_to_files = true,
            .write_logs_to_console = false,
            .write_alerts_to_console = true,
            .component_logging_default = true,
            .max_directory_mb = 100,
            .retention_days = 30,
            .directory = default_directory,
            .components_directory = default_directory / "components",
            .report = {},
            .experimental = true
        };

        const auto disable_all = parse_bool(raw.disable_all);
        settings.disable_all = disable_all.value_or(false);
        append_bool(
            settings.report,
            "disable_all",
            !disable_all,
            "off"
        );

        settings.directory = ac::paths::detail::resolve_configured_directory(
            raw.directory,
            default_directory,
            installation_root
        );
        settings.components_directory = settings.directory / "components";
        const auto max_directory_mb = parse_non_negative(raw.max_directory_mb);
        settings.max_directory_mb = max_directory_mb.value_or(100);
        append_integer(
            settings.report,
            "max_directory_mb",
            !max_directory_mb,
            100
        );

        const auto retention_days = parse_non_negative(raw.retention_days);
        settings.retention_days = retention_days.value_or(30);
        append_integer(
            settings.report,
            "retention_days",
            !retention_days,
            30
        );

        const auto level = parse_level(raw.level);
        settings.level = level.value_or(LogLevel::debug);
        if (!level) {
            settings.report += "level missing or invalid; using debug\n";
        }

        const auto log_console_output = parse_bool(raw.log_console_output);
        settings.log_console_output = log_console_output.value_or(true);
        append_bool(
            settings.report,
            "log_console_output",
            !log_console_output,
            "on"
        );

        const auto write_files = parse_bool(raw.write_logs_to_files);
        settings.write_logs_to_files = write_files.value_or(true);
        append_bool(
            settings.report,
            "write_logs_to_files",
            !write_files,
            "on"
        );

        const auto write_console = parse_bool(raw.write_logs_to_console);
        settings.write_logs_to_console = write_console.value_or(false);
        append_bool(
            settings.report,
            "write_logs_to_console",
            !write_console,
            "off"
        );

        const auto write_alerts = parse_bool(raw.write_alerts_to_console);
        settings.write_alerts_to_console = write_alerts.value_or(true);
        append_bool(
            settings.report,
            "write_alerts_to_console",
            !write_alerts,
            "on"
        );

        const auto family_default = parse_bool(raw.component_logging_default);
        settings.component_logging_default = family_default.value_or(true);
        append_bool(
            settings.report,
            "component_logging_default",
            !family_default,
            "on"
        );

        const auto experimental = parse_bool(raw.experimental);
        settings.experimental = experimental.value_or(true);
        append_bool(
            settings.report,
            "experimental",
            !experimental,
            "on"
        );

        return settings;
    }

    bool resolve_component_logging(
        const LoggingFallback fallback,
        const std::optional<std::string_view>& logging,
        const bool component_logging_default
    ) noexcept {
        if (const auto value = parse_bool(logging)) {
            return *value;
        }
        if (fallback == LoggingFallback::off) {
            return false;
        }
        return component_logging_default;
    }

} // namespace ac::logging::config::detail
