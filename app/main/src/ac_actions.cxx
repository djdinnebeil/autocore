module ac_actions;

import std;

import auto_core.clipboard;
import auto_core.clock;
import auto_core.encoding;
import ac_main;

import <Windows.h>;

/**
    * \brief Prints the current timestamp.
    *
    * This function prints the current timestamp to the screen.
    * \keymap_command
    */
void print_timestamp() {
    auto_core.print_and_insert(ac::clock::get_timestamp());
}

/**
    * \brief Prints the extended timestamp.
    *
    * This function prints the extended timestamp to the screen.
    * \keymap_command
    */
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
    * \brief Prints the current date and time as YYYY-MM-DD � HH:MM
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
    * \brief Prints the current date and time as YYYY-MM-DD � HH:MM
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
    auto clipboard_text = auto_core.get_clipboard_text();

    if (!clipboard_text) {
        return;
    }

    std::wstring clipboard_item =
        L"[" + *clipboard_text + L"]";

    auto_core.print_and_insert_text_replacing_clipboard(
        clipboard_item
    );
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
    auto_core.print_and_insert(std::string {"Testing std1::string: caf——Auto Core"});
}

/** \keymap_command */
void print_and_insert_special_utf16() {
    auto_core.print_and_insert(std::wstring {L"Testing std2::wstring: caf——Auto Core"});
}

/** \keymap_command */
void print_and_insert_testing() {
    print_and_insert_special_utf8();
    Sleep(100); 
    print_and_insert_special_utf16();
}

/** \keymap_command */
void encoding_test() {
    auto_core.print("Entering encoding_test");

    const std::wstring original =
        L"Testing UTF-16: caf\u00e9\u2014Auto Core \U0001f600";

    const std::string utf8 = ac::encoding::to_utf8(original);
    const std::wstring round_trip = ac::encoding::to_utf16(utf8);

    auto_core.print(
        "UTF-16 -> UTF-8 -> UTF-16: {}",
        round_trip == original ? "PASS" : "FAIL"
    );

    auto_core.print(round_trip);

    std::string invalid_utf8{
        static_cast<char>(0xC3),
        static_cast<char>(0x28)
    };

    try {
        auto result = ac::encoding::to_utf16(invalid_utf8);
        std::cout << "Conversion succeeded unexpectedly\n";
    }
    catch (const std::exception& e) {
        std::cout << "Conversion failed: " << e.what() << '\n';
    }

    std::string user_like_input{
    'c',
    'a',
    'f',
    static_cast<char>(0xE9)
};

    try {
        auto result = ac::encoding::to_utf16(user_like_input);
        std::cout << "Conversion succeeded\n";
    }
    catch (const std::exception& e) {
        std::cout << "Conversion failed: " << e.what() << '\n';
    }
}

/** \keymap_command */
void send_crash_command() {
    *(int*)0 = 0;
}

void ac_actions::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("print_timestamp", &::print_timestamp);
    registry.add("print_date_iso", &::print_date_iso);
    registry.add("print_date_compact", &::print_date_compact);
    registry.add("print_date_iso_with_timestamp", &::print_date_iso_with_timestamp);
    registry.add("print_date_iso_with_timestamp_w", &::print_date_iso_with_timestamp_w);
    registry.add("add_brackets_around_clipboard", &::add_brackets_around_clipboard);
    registry.add("send_alt_f12", &::send_alt_f12);
    registry.add("print_and_insert_special_utf8", &::print_and_insert_special_utf8);
    registry.add("print_and_insert_special_utf16", &::print_and_insert_special_utf16);
    registry.add("print_and_insert_testing", &::print_and_insert_testing);
    registry.add("encoding_test", &::encoding_test);
    registry.add("send_crash_command", &::send_crash_command);
}
