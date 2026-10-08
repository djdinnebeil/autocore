/**
 * \file writer_protocol.ixx
 * \brief Writer command names used by `writer_ac.exe`.
 */
export module writer_protocol;

import std;

export namespace ac::protocol::writer {

/** A keymap command name forwarded as an invoke payload. */
struct Command {
    std::string_view name;
};

inline constexpr Command select_and_insert_session_prompt {
    "select_and_insert_session_prompt"
};
inline constexpr Command create_new_note_in_notepad {
    "create_new_note_in_notepad"
};
inline constexpr Command launch_task_list {"launch_task_list"};
inline constexpr Command print_timestamp {"print_timestamp"};
inline constexpr Command print_date_iso {"print_date_iso"};
inline constexpr Command print_date_compact {"print_date_compact"};
inline constexpr Command print_date_iso_with_timestamp {
    "print_date_iso_with_timestamp"
};
inline constexpr Command print_date_iso_with_timestamp_w {
    "print_date_iso_with_timestamp_w"
};
inline constexpr Command add_brackets_around_clipboard {
    "add_brackets_around_clipboard"
};
inline constexpr Command print_and_insert_special_utf8 {
    "print_and_insert_special_utf8"
};
inline constexpr Command print_and_insert_special_utf16 {
    "print_and_insert_special_utf16"
};
inline constexpr Command print_and_insert_testing {
    "print_and_insert_testing"
};

inline constexpr std::array commands {
    select_and_insert_session_prompt,
    create_new_note_in_notepad,
    launch_task_list,
    print_timestamp,
    print_date_iso,
    print_date_compact,
    print_date_iso_with_timestamp,
    print_date_iso_with_timestamp_w,
    add_brackets_around_clipboard,
    print_and_insert_special_utf8,
    print_and_insert_special_utf16,
    print_and_insert_testing,
};

} // namespace ac::protocol::writer
