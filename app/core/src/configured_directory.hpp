/**
 * \file configured_directory.hpp
 * \brief Relative/absolute INI directory resolution against the executable.
 */
#pragma once

#include <filesystem>
#include <optional>

namespace ac::paths::detail {

    [[nodiscard]] std::filesystem::path resolve_configured_directory(
        const std::optional<std::filesystem::path>& configured,
        const std::filesystem::path& default_directory,
        const std::filesystem::path& executable_directory
    );

} // namespace ac::paths::detail
