/**
 * \file logging_config_detail.hpp
 * \brief Pure resolution of logger configuration values.
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace ac::logging::config::detail {

    struct RawSettings {
        std::optional<std::string_view> merge_interval_seconds;
        std::optional<std::string_view> merge_logs_on_shutdown;
        std::optional<std::string_view> write_logs_to_console;
        std::optional<std::filesystem::path> directory;
    };

    struct Settings {
        std::uint64_t merge_interval_seconds;
        bool merge_logs_on_shutdown;
        bool write_logs_to_console;
        std::filesystem::path directory;
        std::filesystem::path components_directory;
        std::string report;
    };

    Settings resolve(
        const RawSettings& raw,
        const std::filesystem::path& default_directory,
        const std::filesystem::path& installation_root
    );

} // namespace ac::logging::config::detail
