/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/taskbar_policy.ini`.
 */
export module taskbar_defaults;

import std;

export namespace taskbar::defaults {

    constexpr std::string_view mode_live = "live";
    constexpr std::string_view mode_cache = "cache";
    constexpr std::string_view mode = mode_live;

    constexpr std::string_view ini_text =
        "[taskbar]\n"
        "mode = live\n"
        "logging = on\n";

    [[nodiscard]]
    inline bool is_mode(const std::string_view value) noexcept {
        return value == mode_live || value == mode_cache;
    }

    [[nodiscard]]
    inline std::string ini_for(
        const std::string_view stored_mode,
        const bool logging = true
    ) {
        const std::string mode_value =
            stored_mode == mode_cache
                ? std::string {mode_cache}
                : std::string {mode_live};
        return "[taskbar]\nmode = " + mode_value +
            "\nlogging = " + (logging ? "on" : "off") + "\n";
    }

} // namespace taskbar::defaults
