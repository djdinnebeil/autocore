/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/server.ini`.
 */
export module server_defaults;

import std;

export namespace server::defaults {

    constexpr int port = 8585;
    constexpr std::string_view document_root = "components\\server";

    constexpr std::string_view ini_text =
        "[server]\n"
        "port = 8585\n"
        "document_root = components\\server\n";

    [[nodiscard]]
    inline std::string ini_for(std::string_view root, int configured_port) {
        const std::string stored_root =
            root.empty() ? std::string {document_root} : std::string {root};
        if (stored_root == document_root && configured_port == port) {
            return std::string {ini_text};
        }
        return std::string {
            "[server]\n"
            "port = "
        } + std::to_string(configured_port) +
            "\n"
            "document_root = " +
            stored_root +
            "\n";
    }

} // namespace server::defaults
