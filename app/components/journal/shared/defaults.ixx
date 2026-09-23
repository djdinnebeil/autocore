/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/journal.ini`.
 */
export module journal_defaults;

import std;

export namespace journal::defaults {

    constexpr std::string_view directory = "components\\journal";
    constexpr int day_rollover_hour = 0;

    constexpr std::string_view ini_text =
        "[journal]\n"
        "directory = components\\journal\n"
        "series =\n"
        "remote_sync = disable\n"
        "\n"
        "[timestamp]\n"
        "day_rollover_hour = 0\n";

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
            "[journal]\n"
            "directory = "
        } + value +
            "\n"
            "series =\n"
            "remote_sync = disable\n"
            "\n"
            "[timestamp]\n"
            "day_rollover_hour = 0\n";
    }

} // namespace journal::defaults
