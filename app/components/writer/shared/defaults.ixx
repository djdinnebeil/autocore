/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/writer.ini`.
 */
export module writer_defaults;

import std;

export namespace writer::defaults {

    constexpr std::string_view directory = "writer";
    constexpr std::string_view notes_directory = "notes";

    constexpr std::string_view ini_text =
        "[writer]\n"
        "directory = writer\n"
        "notes_directory = notes\n";

    [[nodiscard]]
    inline std::string ini_for(
        std::string_view stored_directory,
        std::string_view stored_notes
    ) {
        const std::string value =
            stored_directory.empty()
                ? std::string {directory}
                : std::string {stored_directory};
        const std::string notes =
            stored_notes.empty()
                ? std::string {notes_directory}
                : std::string {stored_notes};
        if (value == directory && notes == notes_directory) {
            return std::string {ini_text};
        }
        return std::string {
            "[writer]\n"
            "directory = "
        } + value +
            "\n"
            "notes_directory = " +
            notes +
            "\n";
    }

} // namespace writer::defaults
