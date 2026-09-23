/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/taskbar.ini`.
 */
export module taskbar_defaults;

import std;

export namespace taskbar::defaults {

    constexpr std::string_view directory = "taskbar";
    constexpr std::string_view mode = "live";

    constexpr std::string_view ini_text =
        "[taskbar]\n"
        "directory = taskbar\n"
        "mode = live\n";

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
            "[taskbar]\n"
            "directory = "
        } + value +
            "\n"
            "mode = live\n";
    }

} // namespace taskbar::defaults
