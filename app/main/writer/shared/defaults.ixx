/**
 * \file defaults.ixx
 * \brief Portable defaults for `config/writer.ini`.
 */
export module writer_defaults;

import std;

export namespace writer::defaults {

    constexpr std::string_view directory = "writer";
    constexpr std::string_view notes_subdirectory = "notes";

    constexpr std::string_view session_prompt_primary =
        "I am running Auto Core.";
    constexpr std::string_view session_prompt_secondary =
        "I am installing a new component in Auto Core.";
    constexpr std::string_view task_line =
        "Add a new task to task_list.txt to show up here.";

    constexpr std::string_view session_prompts_text =
        "I am running Auto Core.\n"
        "I am installing a new component in Auto Core.\n";

    constexpr std::string_view task_list_text =
        "Add a new task to task_list.txt to show up here.\n";

    constexpr std::string_view ini_text =
        "[writer]\n"
        "directory = writer\n"
        "notes_subdirectory = notes\n"
        "logging = on\n";

    [[nodiscard]]
    inline std::string ini_for(
        std::string_view stored_directory,
        std::string_view stored_notes,
        const bool logging = true
    ) {
        const std::string value =
            stored_directory.empty()
                ? std::string {directory}
                : std::string {stored_directory};
        const std::string notes =
            stored_notes.empty()
                ? std::string {notes_subdirectory}
                : std::string {stored_notes};
        if (value == directory && notes == notes_subdirectory && logging) {
            return std::string {ini_text};
        }
        return std::string {
            "[writer]\n"
            "directory = "
        } + value +
            "\n"
            "notes_subdirectory = " +
            notes +
            "\nlogging = " +
            (logging ? "on" : "off") +
            "\n";
    }

} // namespace writer::defaults
