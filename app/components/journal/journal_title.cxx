module;

#include <Windows.h>

module journal_title;

import std;
import auto_core.core.clock;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.keyboard;
import auto_core.core.paths;
import journal_clock;
import journal_cloud;
import journal_component;
import journal_database;

namespace {

void save_file() {
    INPUT inputs[4] {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'S';
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'S';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

void create_new_file() {
    INPUT inputs[4] {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'N';
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'N';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

std::wstring foreground_window_title() {
    const HWND window = GetForegroundWindow();
    const int length = GetWindowTextLengthW(window);
    if (length <= 0) {
        return {};
    }

    std::wstring title(static_cast<std::size_t>(length) + 1, L'\0');
    const int copied = GetWindowTextW(window, title.data(), length + 1);
    title.resize(copied > 0 ? static_cast<std::size_t>(copied) : 0);
    return title;
}

std::optional<std::string> journal_setting(std::string_view name) {
    const auto document = ac::ini::read(
        ac::paths::config_directory() / "journal.ini"
    );
    if (!document) {
        return std::nullopt;
    }
    if (const auto value = document->find("journal", name)) {
        return std::string {*value};
    }
    return std::nullopt;
}

std::string trim_copy(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return std::string {value.substr(first, last - first + 1)};
}

std::string ascii_lower(std::string value) {
    for (char& character : value) {
        if (character >= 'A' && character <= 'Z') {
            character = static_cast<char>(character + ('a' - 'A'));
        }
    }
    return value;
}

bool remote_sync_enabled() {
    const std::string value = ascii_lower(
        trim_copy(journal_setting("remote_sync").value_or(""))
    );
    return value == "enable" || value == "enabled";
}

std::string episode_title() {
    const std::string series = trim_copy(journal_setting("series").value_or(""));
    const auto episode = journal_database::take_next_episode(series);
    if (!episode) {
        journal_component().log_and_print("{}", episode.error());
        return {};
    }

    const std::string name_and_number = std::format(
        "{} {}", episode->name, episode->number
    );
    if (remote_sync_enabled()) {
        update_string_in_firebase(name_and_number);
    }

    return std::format(
        "{}\n{}\n\n{}",
        name_and_number,
        ac::clock::get_date_compact(),
        journal_clock::get_extended_timestamp()
    );
}

} // namespace

void journal_title::print_episode_title() {
    const std::string text = episode_title();
    if (text.empty()) {
        if (!ac::keyboard::send_linebreak()) {
            journal_component().log_and_print(
                "Unable to send a linebreak."
            );
        }
        return;
    }
    const std::wstring title = ac::encoding::to_utf16(text);
    journal_component().print(title);
    journal_component().insert_text_preserving_clipboard_text(title + L"\n\n");
    save_file();
}

void journal_title::save_file_and_create_new_file() {
    const std::wstring previous_title = foreground_window_title();
    save_file();
    Sleep(50);
    create_new_file();
    Sleep(300);
    const std::wstring new_title = foreground_window_title();
    if (!new_title.empty() && new_title != previous_title) {
        journal_title::print_episode_title();
    }
}
