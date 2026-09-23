module writer_commands;

import std;
import writer_protocol;

command_registry::Registry create_writer_command_registry() {
    command_registry::Registry registry;
    registry.add(
        std::string {ac::protocol::writer::select_and_insert_gpt_prompt.name},
        writer_actions::select_and_insert_gpt_prompt
    );
    registry.add(
        std::string {ac::protocol::writer::create_new_note_in_notepad.name},
        writer_actions::create_new_note_in_notepad
    );
    registry.add(
        std::string {ac::protocol::writer::launch_task_list.name},
        writer_actions::launch_task_list
    );
    registry.add(
        std::string {ac::protocol::writer::print_timestamp.name},
        writer_actions::print_timestamp
    );
    registry.add(
        std::string {ac::protocol::writer::print_date_iso.name},
        writer_actions::print_date_iso
    );
    registry.add(
        std::string {ac::protocol::writer::print_date_compact.name},
        writer_actions::print_date_compact
    );
    registry.add(
        std::string {
            ac::protocol::writer::print_date_iso_with_timestamp.name
        },
        writer_actions::print_date_iso_with_timestamp
    );
    registry.add(
        std::string {
            ac::protocol::writer::print_date_iso_with_timestamp_w.name
        },
        writer_actions::print_date_iso_with_timestamp_w
    );
    registry.add(
        std::string {
            ac::protocol::writer::add_brackets_around_clipboard.name
        },
        writer_actions::add_brackets_around_clipboard
    );
    registry.add(
        std::string {
            ac::protocol::writer::print_and_insert_special_utf8.name
        },
        writer_actions::print_and_insert_special_utf8
    );
    registry.add(
        std::string {
            ac::protocol::writer::print_and_insert_special_utf16.name
        },
        writer_actions::print_and_insert_special_utf16
    );
    registry.add(
        std::string {
            ac::protocol::writer::print_and_insert_testing.name
        },
        writer_actions::print_and_insert_testing
    );
    return registry;
}
