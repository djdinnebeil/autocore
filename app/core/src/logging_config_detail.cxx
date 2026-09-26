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
    }

    Settings resolve(
        const RawSettings& raw,
        const std::filesystem::path& default_directory,
        const std::filesystem::path& installation_root
    ) {
        Settings settings {
            .merge_interval_seconds = 60,
            .merge_logs_on_shutdown = true,
            .write_logs_to_console = false,
            .directory = default_directory,
            .components_directory = default_directory / "components",
            .report = "Logging configuration:\n"
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

        if (const auto value = parse_bool(raw.write_logs_to_console)) {
            settings.write_logs_to_console = *value;
        }
        else {
            settings.report +=
                "write_logs_to_console missing or invalid; using off\n";
        }
        settings.report += settings.write_logs_to_console
            ? "write_logs_to_console = on\n"
            : "write_logs_to_console = off\n";

        settings.directory = ac::paths::detail::resolve_configured_directory(
            raw.directory,
            default_directory,
            installation_root
        );
        settings.components_directory = settings.directory / "components";
        settings.report += "logger settings loaded\n";
        return settings;
    }

} // namespace ac::logging::config::detail
