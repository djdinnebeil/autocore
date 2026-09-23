/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/spotify.ini`.
 */
export module spotify_defaults;

import std;

export namespace spotify::defaults {

    constexpr std::string_view directory = "components\\spotify";

    constexpr std::string_view ini_text =
        "[spotify]\n"
        "directory = components\\spotify\n";

    [[nodiscard]]
    inline std::string ini_for(std::string_view stored_directory) {
        const std::string value =
            stored_directory.empty()
                ? std::string {directory}
                : std::string {stored_directory};
        if (value == directory) {
            return std::string {ini_text};
        }
        return std::string {
            "[spotify]\n"
            "directory = "
        } + value +
            "\n";
    }

} // namespace spotify::defaults
