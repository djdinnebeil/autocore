module;

#include <Windows.h>

module journal_title;

import std;
import auto_core.core.clock;
import auto_core.core.encoding;
import auto_core.core.keyboard;
import journal_clock;
import journal_cloud_client;
import journal_component;
import journal_db_client;
import journal_db_protocol;
import journal_episode_format;
import journal_series_map;

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

std::string episode_title() {
    // Allocation does not rewrite series.map. Snapshot freshness is owned by
    // journal_series.exe.
    const auto active = journal::series_map::read_active(
        journal::series_map::file_path()
    );
    const auto choice = journal::series_map::choose_allocate(active);
    auto episode = journal::db::allocate_episode(choice.empty_key ? "" : choice.name);
    if (!episode && !choice.empty_key &&
        journal::db::is_unknown_series(episode.error())) {
        episode = journal::db::allocate_episode("");
    }
    if (!episode) {
        journal_component().log_print("{}", episode.error());
        return {};
    }

    const std::string name_and_number = std::format(
        "{} {}",
        episode->name,
        journal::format_episode_number(episode->allocated, episode->padding)
    );
    if (journal::cloud::service_running()) {
        if (const auto pushed = journal::cloud::push(
                episode->name,
                episode->next_episode
            );
            !pushed) {
            journal_component().log_print("{}", pushed.error());
        }
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
            journal_component().log_print(
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
