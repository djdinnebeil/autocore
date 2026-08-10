module;

#include "ac_api.hpp"

export module paths;

import std;

export namespace ac::paths {

    [[nodiscard]]
    AC_API const std::filesystem::path&
        executable_directory() noexcept;

    [[nodiscard]]
    AC_API const std::filesystem::path&
        config_directory() noexcept;

    [[nodiscard]]
    AC_API const std::filesystem::path&
        error_log_directory() noexcept;

}