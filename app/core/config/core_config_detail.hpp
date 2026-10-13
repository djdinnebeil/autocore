/**
 * \file core_config_detail.hpp
 * \brief Pure resolution of shared warning configuration values.
 */
#pragma once

#include <optional>
#include <string_view>

namespace ac::config::detail {

    struct RawSettings {
        std::optional<std::string_view> missing_winkey_mapping;
        std::optional<std::string_view> unmapped_key;
    };

    struct Settings {
        bool missing_winkey_mapping;
        bool unmapped_key;
    };

    Settings resolve(const RawSettings& raw);

} // namespace ac::config::detail
