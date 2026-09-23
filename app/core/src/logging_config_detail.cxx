#include "configured_directory.hpp"
#include "logging_config_detail.hpp"

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
    }

    Settings resolve(
        const RawSettings& raw,
        const std::filesystem::path& default_directory,
        const std::filesystem::path& installation_root
    ) {
        Settings settings {
            .write_to_console = false,
            .directory = default_directory,
            .components_directory = default_directory / "components",
            .report = "Logging configuration:\n"
        };

        if (const auto value = parse_bool(raw.write_to_console)) {
            settings.write_to_console = *value;
        }
        else {
            settings.report +=
                "write_to_console missing or invalid; using false\n";
        }

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
