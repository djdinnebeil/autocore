/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/dash.ini`.
 *
 * The vault stays under `%LOCALAPPDATA%\Auto Core`. `logging` is the family
 * switch; the compiled default is off.
 */
export module dash_defaults;

import std;

export namespace dash::defaults {

    constexpr std::string_view ini_text =
        "[dash]\n"
        "logging = off\n";

    [[nodiscard]]
    inline std::string ini_for(const bool logging) {
        return logging
            ? std::string {"[dash]\nlogging = on\n"}
            : std::string {ini_text};
    }

} // namespace dash::defaults
