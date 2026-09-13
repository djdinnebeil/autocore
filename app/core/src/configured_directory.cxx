#include "configured_directory.hpp"

namespace ac::paths::detail {

    std::filesystem::path resolve_configured_directory(
        const std::optional<std::filesystem::path>& configured,
        const std::filesystem::path& default_directory,
        const std::filesystem::path& executable_directory
    ) {
        if (!configured || configured->empty()) {
            return default_directory;
        }

        std::filesystem::path resolved {*configured};
        if (resolved.is_relative()) {
            resolved = executable_directory / resolved;
        }
        return resolved.lexically_normal();
    }

} // namespace ac::paths::detail
