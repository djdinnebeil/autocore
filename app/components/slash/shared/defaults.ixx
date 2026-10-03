/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/slash.ini`.
 *
 * `mode` selects the recycle-bin report. `verbose` is today's categorized
 * report and the compiled default.
 */
export module slash_defaults;

import std;

export namespace slash::defaults {

    constexpr std::string_view mode_verbose = "verbose";
    constexpr std::string_view mode_concise = "concise";
    constexpr std::string_view mode_silent = "silent";

    constexpr std::string_view ini_text =
        "[slash]\n"
        "mode = verbose\n"
        "logging = on\n";

    [[nodiscard]]
    constexpr bool is_mode(std::string_view value) noexcept {
        return
            value == mode_verbose ||
            value == mode_concise ||
            value == mode_silent;
    }

    [[nodiscard]]
    inline std::string ini_for_mode(
        std::string_view mode,
        const bool logging = true
    ) {
        if (mode == mode_verbose && logging) {
            return std::string {ini_text};
        }
        std::string text;
        text.append("[slash]\nmode = ");
        text.append(mode);
        text.append("\nlogging = ");
        text.append(logging ? "on" : "off");
        text.push_back('\n');
        return text;
    }

} // namespace slash::defaults
