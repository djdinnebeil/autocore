/**
 * \file core_config_detail.hpp
 * \brief Pure resolution of shared Auto Core configuration values.
 */
#pragma once

#include <optional>
#include <string_view>

namespace ac::config::detail {

    struct RawSettings {
        std::optional<std::string_view> warn_without_winkey_mapping;
    };

    struct Settings {
        bool warn_without_winkey_mapping;
    };

    Settings resolve(const RawSettings& raw);

} // namespace ac::config::detail
