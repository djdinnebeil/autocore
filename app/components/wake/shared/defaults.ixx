/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/wake.ini`.
 *
 * `[wake] directory` is Wake's operational history folder. Ordinary
 * component logs stay under the shared logging tree.
 */
export module wake_defaults;

import std;

export namespace wake::defaults {

    constexpr std::string_view directory = "components\\wake";

    constexpr std::string_view ini_text =
        "[wake]\n"
        "directory = components\\wake\n"
        "logging = on\n";

    [[nodiscard]]
    inline std::string ini_for(
        const std::string_view stored_directory,
        const bool logging = true
    ) {
        const std::string value = stored_directory.empty()
            ? std::string {directory}
            : std::string {stored_directory};
        if (value == directory && logging) {
            return std::string {ini_text};
        }
        return "[wake]\ndirectory = " + value +
            "\nlogging = " + (logging ? "on" : "off") + "\n";
    }

} // namespace wake::defaults
