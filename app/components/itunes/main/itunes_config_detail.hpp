/**
 * \file itunes_config_detail.hpp
 * \brief Pure resolution of iTunes component configuration values.
 */
#pragma once

#include <optional>
#include <string_view>

namespace itunes::config::detail {

    struct RawSettings {
        std::optional<std::string_view> auto_start;
        std::optional<std::string_view> tab_end;
    };

    struct Settings {
        bool auto_start = false;
        int tab_end = 3;
    };

    [[nodiscard]] Settings resolve(
        const RawSettings& raw,
        Settings defaults = {}
    );

} // namespace itunes::config::detail
