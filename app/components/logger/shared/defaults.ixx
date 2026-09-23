/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/logger.ini`.
 */
export module logger_defaults;

import std;

export namespace logger::defaults {

    constexpr std::string_view directory = "logs";
    constexpr bool write_to_console = false;

    constexpr std::string_view ini_text =
        "[logger]\n"
        "directory = logs\n"
        "write_to_console = false\n";

    [[nodiscard]]
    inline std::string ini_for(
        std::string_view stored_directory,
        bool console
    ) {
        const std::string value =
            stored_directory.empty()
                ? std::string {directory}
                : std::string {stored_directory};
        const char* console_text = console ? "true" : "false";
        if (value == directory && !console) {
            return std::string {ini_text};
        }
        return std::string {
            "[logger]\n"
            "directory = "
        } + value +
            "\n"
            "write_to_console = " +
            console_text +
            "\n";
    }

} // namespace logger::defaults
