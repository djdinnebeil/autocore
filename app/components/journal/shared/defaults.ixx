/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/journal.ini`.
 */
export module journal_defaults;

import std;
import journal_remote_sync;

export namespace journal::defaults {

    constexpr std::string_view directory = "components\\journal";

    constexpr std::string_view ini_text =
        "[journal]\n"
        "directory = components\\journal\n"
        "auto_select_new_series = on\n"
        "remote_sync = off\n"
        "logging = on\n";

    [[nodiscard]]
    inline std::string ini_for(
        std::string_view stored_directory,
        const bool logging = true,
        const bool auto_select_new_series = true,
        std::string_view remote_sync = "off"
    ) {
        const std::string value =
            stored_directory.empty()
                ? std::string {directory}
                : std::string {stored_directory};
        const auto token = journal::remote_sync::canonical(remote_sync);
        const std::string sync = token ? *token : std::string {"off"};
        if (value == directory && logging && auto_select_new_series && sync == "off") {
            return std::string {ini_text};
        }
        return std::string {
            "[journal]\n"
            "directory = "
        } + value +
            "\nauto_select_new_series = " +
            std::string {auto_select_new_series ? "on" : "off"} +
            "\nremote_sync = " + sync +
            "\nlogging = " +
            std::string {logging ? "on" : "off"} +
            "\n";
    }

} // namespace journal::defaults
