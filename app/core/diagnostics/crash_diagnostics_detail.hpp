#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace ac::crash::detail {

    inline constexpr bool diagnostics_default = true;

    [[nodiscard]]
    constexpr bool resolve_enabled(
        const std::optional<std::string_view> value
    ) noexcept {
        if (!value) {
            return diagnostics_default;
        }
        if (*value == "off") {
            return false;
        }
        return true;
    }

    [[nodiscard]]
    inline std::string event_directory_name(
        const std::string_view timestamp,
        const std::string_view executable_stem,
        const unsigned long process_id,
        const unsigned collision = 0
    ) {
        std::string name;
        name.reserve(timestamp.size() + executable_stem.size() + 24);
        name.append(timestamp);
        name.push_back('_');
        name.append(executable_stem);
        name.push_back('_');
        name.append(std::to_string(process_id));
        if (collision != 0) {
            name.push_back('_');
            name.append(std::to_string(collision));
        }
        return name;
    }

} // namespace ac::crash::detail
