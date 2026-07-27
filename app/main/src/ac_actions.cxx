module ac_actions;
import std;
import visual;
import <Windows.h>;

/**
    * \brief Prints the current timestamp.
    *
    * This function prints the current timestamp to the screen.
    * \runtime
    */
void print_timestamp() {
    std::wstring most_recent_clipboard_text = ac::clipboard::get_clipboard_text();
    Sleep(50);
    ac::print_to_screen(ac::clock::get_timestamp());
    Sleep(100);
    ac::clipboard::set_clipboard_text(most_recent_clipboard_text);
}

/**
    * \brief Prints the current datestamp.
    *
    * This function prints the current datestamp to the screen.
    * \runtime
    */
void print_datestamp() {
    ac::print_to_screen(ac::clock::get_datestamp());
}

/**
    * \brief Prints the current datestamp in ISO 8601 format.
    *
    * This function prints the current datestamp to the screen.
    * \runtime
    */
void print_datestamp_iso() {
    ac::print_to_screen(ac::clock::get_datestamp_iso());
}

/**
    * \brief Prints the current date and time as YYYY-MM-DD – HH:MM
    *
    * This function prints the current date and time to the screen.
    * \runtime
    */
void print_datestamp_iso_with_timestamp() {
    ac::print_to_screen(ac::clock::get_datestamp_iso_with_timestamp());
}

/**
    * \brief Prints the current date and time as YYYY-MM-DD – HH:MM
    *
    * This function prints the current date and time to the screen.
    * \runtime
    */
void print_datestamp_iso_with_timestamp_w() {
    ac::print_to_screen_w(ac::utils::str_to_wstr(ac::clock::get_datestamp_iso()) + L" \u2013 " + ac::utils::str_to_wstr(ac::clock::get_timestamp()));
}

/**
 * \brief Prints the current day of the week.
 */
void print_today_is_day() {
    ac::print("Today is {}", ac::clock::get_day_of_week());
}

/**
    * \brief Adds brackets around the clipboard text.
    * \runtime
    * This function adds brackets around the current text in the clipboard and pastes it.
    */
void add_brackets_around_clipboard() {
    std::wstring clipboard_item = L"[" + ac::clipboard::get_clipboard_text() + L"]";
    ac::print(clipboard_item);
    ac::clipboard::set_clipboard_text(clipboard_item);
    ac::clipboard::paste_from_clipboard();
}

/**
    * \brief Sends keyboard event of 'alt + f12'
    *
    * This event launches a terminal window in WebStorm.
    * \runtime
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
