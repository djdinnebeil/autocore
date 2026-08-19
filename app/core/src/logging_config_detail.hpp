/**
 * \file logging_config_detail.hpp
 * \brief Pure resolution of logger configuration values.
 */
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace ac::logging::config::detail {

    struct RawSettings {
        std::optional<std::string_view> enabled;
        std::optional<std::string_view> write_to_console;
        std::optional<std::filesystem::path> directory;
    };

    struct Settings {
        bool enabled;
        bool write_to_console;
        std::filesystem::path directory;
        std::string report;
    };

    Settings resolve(
        const RawSettings& raw,
        const std::filesystem::path& default_directory,
        const std::filesystem::path& executable_directory
    );

} // namespace ac::logging::config::detail
