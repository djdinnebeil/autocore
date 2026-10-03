/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/itunes.ini` and `song.format`.
 */
module;

#include "song_template_detail.hpp"

export module itunes_defaults;

import std;

export namespace itunes::defaults {

    constexpr std::string_view directory = "components/itunes";
    constexpr bool auto_start = true;
    constexpr std::string_view song_format = itunes::song::detail::default_template;

    constexpr std::string_view ini_text =
        "[itunes]\n"
        "directory = components/itunes\n"
        "auto_start = on\n"
        "logging = on\n";

    [[nodiscard]] inline std::string ini_for(
        std::string_view stored_directory,
        const bool stored_auto_start,
        const bool logging = true
    ) {
        std::string text = "[itunes]\ndirectory = ";
        text += stored_directory.empty() ? directory : stored_directory;
        text += "\nauto_start = ";
        text += stored_auto_start ? "on" : "off";
        text += logging ? "\nlogging = on\n" : "\nlogging = off\n";
        return text;
    }

} // namespace itunes::defaults
