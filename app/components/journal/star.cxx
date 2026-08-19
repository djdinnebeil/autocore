module;

#include <sqlite3.h>
#include <Windows.h>

module star;

import std;
import auto_core.clock;
import auto_core.encoding;
import auto_core.ini;
import auto_core.paths;
import cloud;
import journal_clock;
import journal_component;

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

std::optional<std::string> star_setting(std::string_view name) {
    const auto document = ac::ini::read(
        ac::paths::config_directory() / "star.ini"
    );
    if (!document) {
        return std::nullopt;
    }
    if (const auto value = document->find("star", name)) {
        return std::string {*value};
    }
    return std::nullopt;
}

std::string star_name() {
    const std::string value = star_setting("journal_name").value_or("");
    journal_component().logg_and_logg("journal name: {}", value);
    return value;
}

std::string database_path() {
    const std::string value = star_setting("database_path").value_or("");
    journal_component().logg_and_logg("database path: {}", value);
    return value;
}

int episode_number() {
    static const std::string path = database_path();
    sqlite3* database = nullptr;
    if (sqlite3_open(path.c_str(), &database) != SQLITE_OK) {
        if (database) sqlite3_close(database);
        return -1;
    }

    sqlite3_stmt* select_statement = nullptr;
    constexpr const char* select_sql =
        "SELECT value FROM counter LIMIT 1;";
    if (sqlite3_prepare_v2(
            database, select_sql, -1, &select_statement, nullptr
        ) != SQLITE_OK) {
        sqlite3_finalize(select_statement);
        sqlite3_close(database);
        return -1;
    }

    const int number = sqlite3_step(select_statement) == SQLITE_ROW
        ? sqlite3_column_int(select_statement, 0)
        : -1;
    sqlite3_finalize(select_statement);

    if (number != -1) {
        sqlite3_stmt* update_statement = nullptr;
        constexpr const char* update_sql =
            "UPDATE counter SET value = ?;";
        if (sqlite3_prepare_v2(
                database, update_sql, -1, &update_statement, nullptr
            ) == SQLITE_OK) {
            sqlite3_bind_int(update_statement, 1, number + 1);
            sqlite3_step(update_statement);
        }
        sqlite3_finalize(update_statement);
    }

    sqlite3_close(database);
    return number;
}

std::string episode_title() {
    static const std::string name = star_name();
    const std::string name_and_number = std::format(
        "{} {}", name, episode_number()
    );
    update_string_in_firebase(name_and_number);

    return std::format(
        "{}\n{}\n\n{}",
        name_and_number,
        ac::clock::get_date_compact(),
        journal_clock::get_extended_timestamp()
    );
}

} // namespace

void star_actions::print_episode_title() {
    const std::wstring title = ac::encoding::to_utf16(episode_title());
    journal_component().print(title);
    journal_component().insert_text_preserving_clipboard_text(title + L"\n\n");
    save_file();
}

void star_actions::save_file_and_create_new_file() {
    const std::wstring previous_title = foreground_window_title();
    save_file();
    Sleep(50);
    create_new_file();
    Sleep(300);
    const std::wstring new_title = foreground_window_title();
    if (!new_title.empty() && new_title != previous_title) {
        star_actions::print_episode_title();
    }
}
