/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/server.ini` and Server seed files.
 *
 * `port` and `document_root` are seed values for `server_editor.exe`.
 * They are not `server.ini` keys and they are not runtime fallbacks.
 */
export module server_defaults;

import std;

export namespace server::defaults {

    constexpr std::string_view directory = "components\\server";
    constexpr int port = 8585;
    constexpr std::string_view document_root = "site";

    constexpr std::string_view ini_text =
        "[server]\n"
        "directory = components\\server\n"
        "logging = on\n";

    [[nodiscard]]
    inline std::string ini_for(
        std::string_view stored_directory,
        const bool logging = true
    ) {
        const std::string value =
            stored_directory.empty()
                ? std::string {directory}
                : std::string {stored_directory};
        if (value == directory && logging) {
            return std::string {ini_text};
        }
        return std::string {
            "[server]\n"
            "directory = "
        } + value +
            "\nlogging = " +
            (logging ? "on" : "off") +
            "\n";
    }

} // namespace server::defaults
