/**
 * \file itunes_config_detail.hpp
 * \brief Pure resolution of iTunes component configuration values.
 */
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace itunes::config::detail {

    struct RawSettings {
        std::optional<std::string_view> directory;
        std::optional<std::string_view> auto_start;
    };

    struct Settings {
        std::string directory = "components/itunes";
        bool auto_start = true;
    };

    [[nodiscard]] Settings resolve(
        const RawSettings& raw,
        Settings defaults = {}
    );

    /**
     * \brief Resolves `[itunes] directory` against the installation root.
     *
     * A relative path joins `installation_root`. An absolute path is used
     * as-is. A missing key, an empty value, or a path that cannot be
     * constructed uses `<installation_root>/components/itunes`. The
     * directory is not created.
     */
    [[nodiscard]] std::filesystem::path resolve_directory(
        const std::optional<std::string_view>& stored,
        const std::filesystem::path& installation_root
    );

} // namespace itunes::config::detail
