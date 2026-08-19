export module sp_oauth_paths;

import std;

export namespace sp_oauth::paths {

    [[nodiscard]]
    const std::filesystem::path&
        executable_directory() noexcept;

    [[nodiscard]]
    const std::filesystem::path&
        star_directory() noexcept;

}
