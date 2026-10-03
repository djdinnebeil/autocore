#include "configured_directory.hpp"
#include "logging_config_detail.hpp"

#include <charconv>
#include <system_error>

namespace ac::logging::config::detail {

    namespace {
        std::optional<bool> parse_bool(
            const std::optional<std::string_view> value
        ) noexcept {
            if (value == "on" || value == "true") {
                return true;
            }
            if (value == "off" || value == "false") {
                return false;
            }
            return std::nullopt;
        }

        std::optional<LogPrintMode> parse_log_print_mode(
            const std::optional<std::string_view> value
        ) noexcept {
            if (value == "log") {
                return LogPrintMode::log;
            }
            if (value == "print") {
                return LogPrintMode::print;
            }
            return std::nullopt;
        }

        std::optional<std::uint64_t> parse_interval(
            const std::optional<std::string_view> value
        ) noexcept {
            if (!value || value->empty()) {
                return std::nullopt;
            }

            std::int64_t parsed = 0;
            const auto* const end = value->data() + value->size();
            const auto result = std::from_chars(value->data(), end, parsed);
            if (result.ec != std::errc {} || result.ptr != end || parsed < 0) {
                return std::nullopt;
            }
            return static_cast<std::uint64_t>(parsed);
        }

        void append_bool(
            std::string& report,
            const std::string_view key,
            const bool value,
            const bool used_default,
            const std::string_view default_word
        ) {
            if (used_default) {
                report += key;
                report += " missing or invalid; using ";
                report += default_word;
                report += '\n';
            }
            report += key;
            report += " = ";
            report += value ? "on" : "off";
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
            .write_logs_to_files = true,
            .write_logs_to_console = false,
            .log_print_mode = LogPrintMode::print,
            .component_logging_default = true,
            .directory = default_directory,
            .components_directory = default_directory / "components",
            .report = "Logging configuration:\n"
        };

        const auto disable_all = parse_bool(raw.disable_all);
        settings.disable_all = disable_all.value_or(false);
        append_bool(
            settings.report,
            "disable_all",
            settings.disable_all,
            !disable_all,
            "off"
        );

        settings.directory = ac::paths::detail::resolve_configured_directory(
            raw.directory,
            default_directory,
            installation_root
        );
        settings.components_directory = settings.directory / "components";
        settings.report += "directory = " + settings.directory.string() + "\n";

        const auto write_files = parse_bool(raw.write_logs_to_files);
        settings.write_logs_to_files = write_files.value_or(true);
        append_bool(
            settings.report,
            "write_logs_to_files",
            settings.write_logs_to_files,
            !write_files,
            "on"
        );

        const auto write_console = parse_bool(raw.write_logs_to_console);
        settings.write_logs_to_console = write_console.value_or(false);
        append_bool(
            settings.report,
            "write_logs_to_console",
            settings.write_logs_to_console,
            !write_console,
            "off"
        );

        if (const auto mode = parse_log_print_mode(raw.log_print_mode)) {
            settings.log_print_mode = *mode;
        }
        else {
            settings.report += "log_print_mode missing or invalid; using print\n";
        }
        settings.report += settings.log_print_mode == LogPrintMode::log
            ? "log_print_mode = log\n"
            : "log_print_mode = print\n";

        const auto family_default = parse_bool(raw.component_logging_default);
        settings.component_logging_default = family_default.value_or(true);
        append_bool(
            settings.report,
            "component_logging_default",
            settings.component_logging_default,
            !family_default,
            "on"
        );

        settings.report += "logging settings loaded\n";
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

    LoggerSettings resolve_logger(const LoggerRaw& raw) {
        LoggerSettings settings {
            .merge_interval_seconds = 60,
            .merge_logs_on_shutdown = true,
            .report = "Logger configuration:\n"
        };

        if (const auto value = parse_interval(raw.merge_interval_seconds)) {
            settings.merge_interval_seconds = *value;
        }
        else {
            settings.report +=
                "merge_interval_seconds missing or invalid; using 60\n";
        }
        settings.report += "merge_interval_seconds = " +
            std::to_string(settings.merge_interval_seconds) + "\n";

        if (const auto value = parse_bool(raw.merge_logs_on_shutdown)) {
            settings.merge_logs_on_shutdown = *value;
        }
        else {
            settings.report +=
                "merge_logs_on_shutdown missing or invalid; using on\n";
        }
        settings.report += settings.merge_logs_on_shutdown
            ? "merge_logs_on_shutdown = on\n"
            : "merge_logs_on_shutdown = off\n";

        settings.report += "logger settings loaded\n";
        return settings;
    }

} // namespace ac::logging::config::detail
