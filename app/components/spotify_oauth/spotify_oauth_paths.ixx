/**
 * \file spotify_oauth_paths.ixx
 * \brief Executable paths for the Spotify OAuth helper.
 */
export module spotify_oauth_paths;

import std;

export namespace spotify_oauth::paths {

    [[nodiscard]]
    const std::filesystem::path&
        executable_directory() noexcept;

}
