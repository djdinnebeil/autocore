/**
 * \file writer_commands.ixx
 * \brief Runtime command registry for `writer_ac.exe`.
 */
export module writer_commands;

import command_registry;

namespace writer_actions {
    void select_and_insert_session_prompt();
    void create_or_open_daily_note_in_notepad();
    void launch_task_list();
    void print_timestamp();
    void print_date_iso();
    void print_date_compact();
    void print_date_iso_with_timestamp();
    void print_date_iso_with_timestamp_w();
    void add_brackets_around_clipboard();
    void print_and_insert_special_utf8();
    void print_and_insert_special_utf16();
    void print_and_insert_testing();
}

export command_registry::Registry create_writer_command_registry();
