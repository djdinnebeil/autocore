#include "configured_directory.hpp"
#include "spotify_config_detail.hpp"

namespace ac::spotify::config::detail {

    std::filesystem::path resolve(
        const RawSettings& raw,
        const std::filesystem::path& default_directory,
        const std::filesystem::path& installation_root
    ) {
        return ac::paths::detail::resolve_configured_directory(
            raw.directory,
            default_directory,
            installation_root
        );
    }

} // namespace ac::spotify::config::detail
