#include "logging_config_detail.hpp"

namespace ac::logging::config::detail {

    namespace {
        std::optional<bool> parse_bool(
            const std::optional<std::string_view> value
        ) noexcept {
            if (value == "true") {
                return true;
            }
            if (value == "false") {
                return false;
            }
            return std::nullopt;
        }
    }

    Settings resolve(
        const RawSettings& raw,
        const std::filesystem::path& default_directory,
        const std::filesystem::path& executable_directory
    ) {
        Settings settings {
            .enabled = true,
            .write_to_console = false,
            .directory = default_directory,
            .report = "Logging configuration:\n"
        };

        if (const auto value = parse_bool(raw.enabled)) {
            settings.enabled = *value;
        }
        else {
            settings.report += "enabled missing or invalid; using true\n";
        }

        if (const auto value = parse_bool(raw.write_to_console)) {
            settings.write_to_console = *value;
        }
        else {
            settings.report +=
                "write_to_console missing or invalid; using false\n";
        }

        if (raw.directory && !raw.directory->empty()) {
            std::filesystem::path configured {*raw.directory};
            if (configured.is_relative()) {
                configured = executable_directory / configured;
            }
            settings.directory = configured.lexically_normal();
        }

        settings.report += "logger settings loaded\n";
        return settings;
    }

} // namespace ac::logging::config::detail
