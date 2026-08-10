module star;

import std;

import clipboard;
import clock;
import encoding;
import ac_component;
import print;

import cloud;
import <sqlite3.h>;
import <Windows.h>;

/**
 * \brief Simulates a save file operation.
 *
 * This function sends keyboard inputs to simulate the Ctrl+S key combination to save a file.
 */
void save_file() {
    INPUT inputs[4] = {};
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
    INPUT inputs[4] = {};
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

std::wstring get_window_title() {
    HWND current_window_hwnd = GetForegroundWindow();
    int length = GetWindowTextLength(current_window_hwnd);
    if (length == 0) {
        return std::wstring();
    }

    // Allocate buffer of appropriate size
    std::wstring title(length, '\0');
    GetWindowTextW(current_window_hwnd, &title[0], length + 1); // +1 to include null terminator
    return title;
}

/**
 * \keymap_command
 */
void save_file_and_create_new_file() {
    std::wstring current_window_title = get_window_title();
    save_file();
    Sleep(50);
    create_new_file();
    Sleep(300);
    std::wstring new_file_window_title = get_window_title();
    if (new_file_window_title != L"" && (new_file_window_title != current_window_title)) {
        print_episode_title();
    }
}

/**
 * \brief Retrieves the star name from the configuration file.
 *
 * This function reads the star name from the star.ini configuration file.
 *
 * \return The star name as a std::string.
 */
std::string get_star_name() {
    auto_core.logg_and_logg("get_star_name()");
    std::ifstream star_rc(R"(.\config\star.ini)");
    std::string line;
    std::getline(star_rc, line);
    auto_core.logg_and_logg(line);
    return line;
}

/**
 * \brief Retrieves the database path from the configuration file.
 *
 * This function reads the database path from the star.ini configuration file.
 *
 * \return The database path as a std::string.
 */
std::string get_database_path() {
    auto_core.logg_and_logg("get_database_path()");
    std::ifstream star_rc(R"(.\config\star.ini)");
    std::string line;
    std::getline(star_rc, line);  // Skip the star name line
    std::getline(star_rc, line);  // Read the database path line
    auto_core.logg_and_logg(line);
    return line;
}

/**
 * \brief Retrieves and updates the episode number from the database.
 *
 * This function retrieves the current episode number from the SQLite database, increments it,
 * and updates the database with the new episode number.
 *
 * \return The current episode number, or -1 if an error occurs.
 */
int get_episode_number() {
    static std::string db_path = get_database_path();
    sqlite3* db;
    sqlite3_stmt* selectStmt;
    sqlite3_stmt* updateStmt;
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc != SQLITE_OK) {
        return -1;
    }
    const char* selectSQL = "SELECT value FROM counter LIMIT 1;";
    rc = sqlite3_prepare_v2(db, selectSQL, -1, &selectStmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(selectStmt);
        sqlite3_close(db);
        return -1;
    }
    int episode_number;
    if (sqlite3_step(selectStmt) == SQLITE_ROW) {
        episode_number = sqlite3_column_int(selectStmt, 0);
    }
    else {
        episode_number = -1;
    }
    sqlite3_finalize(selectStmt);
    if (episode_number != -1) {
        const char* updateSQL = "UPDATE counter SET value = ?;";
        rc = sqlite3_prepare_v2(db, updateSQL, -1, &updateStmt, nullptr);
        if (rc != SQLITE_OK) {
            sqlite3_finalize(updateStmt);
            sqlite3_close(db);
            return episode_number;
        }
        sqlite3_bind_int(updateStmt, 1, episode_number + 1);
        sqlite3_step(updateStmt);
        sqlite3_finalize(updateStmt);
    }
    sqlite3_close(db);
    return episode_number;
}

/**
 * \brief Generates the episode title.
 *
 * This function generates the episode title using the star name and episode number,
 * updates the title in the cloud, and returns the formatted title std::string.
 *
 * \return The formatted episode title as a std::string.01
 */
std::string get_episode_title() {
    static std::string star_name = get_star_name();
    std::string star_and_number = std::format("{} {}", star_name, get_episode_number());
    update_string_in_firebase(star_and_number);
    std::ostringstream s;
    s << star_and_number << '\n' << ac::clock::get_date_compact() << "\n\n" << ac::clock::get_extended_timestamp();
    return s.str();
}

/**
 * \brief Prints the episode title.
 *
 * This function prints the generated episode title to the screen and saves the file.
 *
 * \keymap_command
 */
void print_episode_title() {
    auto previous_clipboard =
        ac::clipboard::capture_clipboard_text();

    if (!previous_clipboard) {
        auto_core.logg_and_print(
            "Clipboard error: {}",
            ac::clipboard::error_message(previous_clipboard.error())
        );
        return;
    }

    std::wstring episode_title =
        ac::encoding::to_utf16(get_episode_title());

    auto_core.print(episode_title);

    if (auto result =
        ac::clipboard::set_clipboard_text(episode_title + L"\n\n");
        !result) {

        auto_core.logg_and_print(
            "Clipboard error: {}",
            ac::clipboard::error_message(result.error())
        );
        return;
    }

    Sleep(50);

    if (auto result = ac::clipboard::paste_from_clipboard();
        !result) {

        auto_core.logg_and_print(
            "Clipboard error: {}",
            ac::clipboard::error_message(result.error())
        );
        return;
    }

    Sleep(100);

    save_file();

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
