module ac_actions;

import std;

import clipboard;
import clock;
import encoding;
import print;
import ac_component;

import <Windows.h>;

/**
    * \brief Prints the current timestamp.
    *
    * This function prints the current timestamp to the screen.
    * \keymap_command
    */
void print_timestamp() {
    auto previous_clipboard =
        ac::clipboard::capture_clipboard_text();

    if (!previous_clipboard) {
        auto_core.logg_and_print(
            "Clipboard error: {}",
            ac::clipboard::error_message(previous_clipboard.error())
        );
        return;
    }

    Sleep(50);

    auto_core.print_and_insert(ac::clock::get_timestamp());

    Sleep(100);

    if (auto result = ac::clipboard::restore_clipboard_text(
        *previous_clipboard
    ); !result) {
        auto_core.logg_and_print(
            "Clipboard error: {}",
            ac::clipboard::error_message(result.error())
        );
    }
}

/**
    * \brief Prints the extended timestamp.
    *
    * This function prints the extended timestamp to the screen.
    * \keymap_command
    */
void print_extended_timestamp() {
    auto previous_clipboard =
        ac::clipboard::capture_clipboard_text();

    if (!previous_clipboard) {
        auto_core.logg_and_print(
            "Clipboard error: {}",
            ac::clipboard::error_message(previous_clipboard.error())
        );
        return;
    }

    Sleep(50);

    auto_core.print_and_insert(
        ac::clock::get_extended_timestamp()
    );

    Sleep(100);

    if (auto result = ac::clipboard::restore_clipboard_text(
        *previous_clipboard
    ); !result) {
        auto_core.logg_and_print(
            "Clipboard error: {}",
            ac::clipboard::error_message(result.error())
        );
    }
}

/**
    * \brief Prints the current date in ISO format YYYY-MM-DD
    *
    * This function prints the current date in ISO format to the screen.
    * \keymap_command
    */
void print_date_iso() {
    auto_core.print_and_insert(ac::clock::get_date_iso());
}

/**
    * \brief Prints the current date in short format MM-D-YY
    *
    * This function prints the current date in short format to the screen.
    * \keymap_command
    */
void print_date_compact() {
    auto_core.print_and_insert(ac::clock::get_date_compact());
}

/**
    * \brief Prints the current date and time as YYYY-MM-DD – HH:MM
    *
    * This function prints the current date and time to the screen.
    * \keymap_command
    */
void print_date_iso_with_timestamp() {
    const auto datetime = ac::clock::get_local_datetime();
    std::string datetime_str = datetime.date_iso + " - " + datetime.timestamp;
    auto_core.print_and_insert(datetime_str);
}

/**
    * \brief Prints the current date and time as YYYY-MM-DD – HH:MM
    *
    * This function prints the current date and time to the screen.
    * \keymap_command
    */
void print_date_iso_with_timestamp_w() {
    const auto datetime = ac::clock::get_local_datetime();
    std::wstring datetime_wstr = ac::encoding::to_utf16(datetime.date_iso) + L" \u2013 " + ac::encoding::to_utf16(datetime.timestamp);
    auto_core.print_and_insert(datetime_wstr);
}

/**
 * \brief Prints the current day of the week.
 */
void print_today_is_day() {
    auto_core.print("Today is {}", ac::clock::get_day_of_week());
}

/**
 * \brief Adds brackets around the clipboard text.
 * \keymap_command
 * This function adds brackets around the current text in the clipboard and pastes it.
 */
void add_brackets_around_clipboard() {
    auto clipboard_result = ac::clipboard::get_clipboard_text();

    if (!clipboard_result) {
        auto_core.logg_and_print(
            "Clipboard error: {}",
            ac::clipboard::error_message(clipboard_result.error())
        );
        return;
    }

    std::wstring clipboard_item =
        L"[" + *clipboard_result + L"]";

    auto_core.print(clipboard_item);

    if (auto result =
        ac::clipboard::set_clipboard_text(clipboard_item);
        !result) {

        auto_core.logg_and_print(
            "Clipboard error: {}",
            ac::clipboard::error_message(result.error())
        );

        return;
    }

    if (auto result = ac::clipboard::paste_from_clipboard();
        !result) {

        auto_core.logg_and_print(
            "Clipboard error: {}",
            ac::clipboard::error_message(result.error())
        );
    }
}
/**
    * \brief Sends keyboard event of 'alt + f12'
    *
    * This event launches a terminal window in WebStorm.
    * \keymap_command
    */
void send_alt_f12() {
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_MENU; // Virtual key code for Alt
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_F12; // Virtual key code for F12
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_F12;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_MENU;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

/** \keymap_command */
void print_and_insert_special_utf8() {
    auto_core.print_and_insert(std::string {"Testing std1::string: café — Auto Core"});

}

/** \keymap_command */
void print_and_insert_special_utf16() {
    auto_core.print_and_insert(std::wstring {L"Testing std2::wstring: café — Auto Core"});

}

/** \keymap_command */
void print_and_insert_testing() {
    print_and_insert_special_utf8();
    Sleep(100); 
    print_and_insert_special_utf16();

}

/** \keymap_command */
void send_crash_command() {
    *(int*)0 = 0;
}
