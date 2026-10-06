module writer_commands;

import std;
import auto_core.core.clock;
import auto_core.core.encoding;
import writer_component;

void writer_actions::print_timestamp() {
    writer_component().print_and_insert(ac::clock::get_timestamp());
}

void writer_actions::print_date_iso() {
    writer_component().print_and_insert(ac::clock::get_date_iso());
}

void writer_actions::print_date_compact() {
    writer_component().print_and_insert(ac::clock::get_date_compact());
}

void writer_actions::print_date_iso_with_timestamp() {
    const auto datetime = ac::clock::get_local_datetime();
    writer_component().print_and_insert(
        datetime.date_iso + " - " + datetime.timestamp
    );
}

void writer_actions::print_date_iso_with_timestamp_w() {
    const auto datetime = ac::clock::get_local_datetime();
    writer_component().print_and_insert(
        ac::encoding::to_utf16(datetime.date_iso) + L" \u2013 " +
        ac::encoding::to_utf16(datetime.timestamp)
    );
}

void writer_actions::add_brackets_around_clipboard() {
    const auto clipboard_text = writer_component().get_clipboard_text();
    if (!clipboard_text) {
        return;
    }
    writer_component().print_and_insert_text_replacing_clipboard(
        L"[" + *clipboard_text + L"]"
    );
}

void writer_actions::print_and_insert_special_utf8() {
    writer_component().print_and_insert(
        std::string {"Testing std1::string: caf——Auto Core"}
    );
}

void writer_actions::print_and_insert_special_utf16() {
    writer_component().print_and_insert(
        std::wstring {L"Testing std2::wstring: caf——Auto Core"}
    );
}

void writer_actions::print_and_insert_testing() {
    print_and_insert_special_utf8();
    std::this_thread::sleep_for(std::chrono::milliseconds {100});
    print_and_insert_special_utf16();
}
