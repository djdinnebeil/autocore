/**
 * \file provider_display_name.hpp
 * \brief Display labels for keymap command providers.
 *
 * `itunes` stays `iTunes`. Every other id capitalizes its first letter.
 */
#pragma once

#include <string>
#include <string_view>

namespace ac::main {

    [[nodiscard]]
    inline std::string provider_display_name(const std::string_view identity) {
        if (identity == "itunes") {
            return "iTunes";
        }
        std::string name {identity};
        if (!name.empty() && name.front() >= 'a' && name.front() <= 'z') {
            name.front() = static_cast<char>(name.front() - 'a' + 'A');
        }
        return name;
    }

}
