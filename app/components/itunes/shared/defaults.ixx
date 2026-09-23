/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/itunes.ini`.
 */
export module itunes_defaults;

import std;

export namespace itunes::defaults {

    constexpr bool auto_start = false;
    constexpr int tab_end = 3;

    constexpr std::string_view ini_text =
        "[itunes]\n"
        "auto_start = false\n"
        "tab_end = 3\n";

} // namespace itunes::defaults
