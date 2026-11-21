module writer_commands;

import std;
import auto_core.core.encoding;
import writer_component;
import writer_protocol;
import writer_text_commands;

namespace {

void insert_catalog_file(const std::string& filename) {
    std::filesystem::path file;
    try {
        file = writer::text_commands::text_directory() /
            std::filesystem::path {ac::encoding::to_utf16(filename)};
    }
    catch (const std::exception& exception) {
        writer_component().log_alert(
            "Unable to use text file name {}: {}",
            filename,
            exception.what()
        );
        return;
    }

    const auto bytes = writer::text_commands::read_text_file(file);
    if (!bytes) {
        writer_component().log_alert(
            "Unable to read text file: {}",
            file.string()
        );
        return;
    }
    if (bytes->empty()) {
        writer_component().log_alert(
            "Text file is empty: {}",
            file.string()
        );
        return;
    }

    std::wstring text;
    try {
        text = ac::encoding::to_utf16(*bytes);
    }
    catch (const std::exception& exception) {
        writer_component().log_alert(
            "Unable to convert text for insertion: {}",
            exception.what()
        );
        return;
    }

    writer_component().printnl(*bytes);
    writer_component().insert_text_preserving_clipboard_text(
        nullptr,
        text,
        false
    );
}

void register_text_commands(command_registry::Registry& registry) {
    const writer::text_commands::CatalogResult loaded =
        writer::text_commands::read_catalog();
    if (loaded.missing) {
        return;
    }
    for (const std::string& issue : loaded.issues) {
        writer_component().log_alert("{}", issue);
    }
    if (loaded.unavailable) {
        return;
    }

    for (const writer::text_commands::TextCommand& command : loaded.commands) {
        if (registry.contains(command.name)) {
            writer_component().log_alert(
                "{} conflicts with the existing Writer command {}. "
                "It was not added.",
                command.filename,
                command.name
            );
            continue;
        }
        registry.add(
            command.name,
            [name = command.name, filename = command.filename] {
                writer_component().log_info("{}()", name);
                insert_catalog_file(filename);
            },
            command.filename.empty()
                ? std::string {}
                : "Inserts the contents of " + command.filename + "."
        );
    }
}

} // namespace

command_registry::Registry create_writer_command_registry() {
    command_registry::Registry registry;
    registry.add(
        std::string {ac::protocol::writer::select_and_insert_session_prompt.name},
        writer_actions::select_and_insert_session_prompt,
        "Displays prompts from session_prompts.list and inserts the selected prompt."
    );
    registry.add(
        std::string {ac::protocol::writer::select_and_insert_text.name},
        writer_actions::select_and_insert_text,
        "Displays a menu of text files from the text directory configured in writer.ini and inserts the exact contents of the selected file into the active text field."
    );
    registry.add(
        std::string {ac::protocol::writer::select_and_insert_profile_url.name},
        writer_actions::select_and_insert_profile_url,
        "Displays URLs from profile_urls.txt and inserts the selected URL."
    );
    registry.add(
        std::string {ac::protocol::writer::create_new_note_in_notepad.name},
        writer_actions::create_or_open_daily_note_in_notepad,
        "Opens today's note in Notepad, creating it when missing. The folder is notes_subdirectory in writer.ini."
    );
    registry.add(
        std::string {ac::protocol::writer::launch_task_list.name},
        writer_actions::launch_task_list,
        "Opens task_list.txt in Notepad."
    );
    registry.add(
        std::string {ac::protocol::writer::print_timestamp.name},
        writer_actions::print_timestamp,
        "Inserts the local time as HH:MM."
    );
    registry.add(
        std::string {ac::protocol::writer::print_date_iso.name},
        writer_actions::print_date_iso,
        "Inserts the local date as YYYY-MM-DD."
    );
    registry.add(
        std::string {ac::protocol::writer::print_date_compact.name},
        writer_actions::print_date_compact,
        "Inserts the local date as M-D-YY."
    );
    registry.add(
        std::string {
            ac::protocol::writer::print_date_iso_with_timestamp.name
        },
        writer_actions::print_date_iso_with_timestamp,
        "Inserts the local date and time as YYYY-MM-DD - HH:MM."
    );
    registry.add(
        std::string {
            ac::protocol::writer::print_date_iso_with_timestamp_w.name
        },
        writer_actions::print_date_iso_with_timestamp_w,
        "Inserts the local date and time as YYYY-MM-DD \u2013 HH:MM."
    );
    registry.add(
        std::string {
            ac::protocol::writer::add_brackets_around_clipboard.name
        },
        writer_actions::add_brackets_around_clipboard,
        "Inserts the clipboard text wrapped in square brackets."
    );
    register_text_commands(registry);
    return registry;
}
