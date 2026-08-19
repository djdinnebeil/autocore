module taskbar;

import std;
import auto_core.console;
import auto_core.keyboard;
import auto_core.paths;
import ac_main;

import notes;
import <Windows.h>;

namespace {
    std::string_view trim(std::string_view value) {
        const auto first = value.find_first_not_of(" \t\r");
        if (first == std::string_view::npos) {
            return {};
        }

        const auto last = value.find_last_not_of(" \t\r");
        return value.substr(first, last - first + 1);
    }
}

void Taskbar::load_config() {
    positions_.clear();
    std::ifstream input(
        ac::paths::config_directory() / "taskbar.ini"
    );

    if (!input) {
        auto_core.logg_and_print("Unable to open taskbar.ini");
        return;
    }

    bool in_taskbar_section = false;
    std::string line;

    while (std::getline(input, line)) {
        const std::string_view value = trim(line);

        if (value.empty() || value.starts_with(';') ||
            value.starts_with('#')) {
            continue;
        }

        if (value.starts_with('[') && value.ends_with(']')) {
            in_taskbar_section = value == "[taskbar]";
            continue;
        }

        if (!in_taskbar_section) {
            continue;
        }

        const auto equals = value.find('=');
        if (equals == std::string_view::npos) {
            auto_core.logg_and_print(
                "Invalid taskbar setting: {}", value
            );
            continue;
        }

        const std::string_view name = trim(value.substr(0, equals));
        const std::string_view configured_position =
            trim(value.substr(equals + 1));

        int position = -1;
        const auto result = std::from_chars(
            configured_position.data(),
            configured_position.data() + configured_position.size(),
            position
        );

        if (name.empty() || result.ec != std::errc {} ||
            result.ptr != configured_position.data() +
                configured_position.size() ||
            position < 0 || position > 9) {
            auto_core.logg_and_print(
                "Invalid taskbar setting: {}", value
            );
            continue;
        }

        if (positions_.contains(std::string {name})) {
            auto_core.logg_and_print(
                "Taskbar program '{}' is configured more than once.",
                name
            );
            continue;
        }

        const auto duplicate_position = std::ranges::find_if(
            positions_,
            [position](const auto& entry) {
                return entry.second == position;
            }
        );

        if (duplicate_position != positions_.end()) {
            auto_core.logg_and_print(
                "Taskbar position {} is assigned to both '{}' and '{}'.",
                position, duplicate_position->first, name
            );
            continue;
        }

        positions_[std::string {name}] = position;
    }
}

std::optional<int> Taskbar::position(
    const std::string_view program
) const {
    const auto found = positions_.find(std::string {program});
    return found == positions_.end()
        ? std::nullopt
        : std::optional<int> {found->second};
}

void initialize_taskbar() {
    taskbar.load_config();
}

std::optional<int> taskbar_position(
    const std::string_view program
) {
    return taskbar.position(program);
}

BOOL CALLBACK enum_folder_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR  window_title[buffer_size];
    WCHAR  className[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        GetClassNameW(hwnd, className, buffer_size);
        std::wstring wClass(className);
        if (wClass == L"CabinetWClass") {
            taskbar.folder_windows++;
        }
    }
    return TRUE;
}

BOOL CALLBACK enum_word_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR window_title[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        std::wstring title(window_title);
        if (title.length() >= 4 && title.substr(title.length() - 4) == L"Word") {
            taskbar.word_windows += 1;
        }
    }
    return TRUE;
}

BOOL CALLBACK enum_vs_code_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR window_title[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        std::wstring title(window_title);
        if (title.length() >= 18 && title.substr(title.length() - 18) == L"Visual Studio Code") {
            taskbar.vs_code_windows += 1;
        }
    }
    return TRUE;
}

BOOL CALLBACK enum_chrome_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR  window_title[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        std::wstring title(window_title);
        if (title.length() >= 13 && title.substr(title.length() - 13) == L"Google Chrome") {
            taskbar.chrome_windows++;
        }
    }
    return TRUE;
}

BOOL CALLBACK enum_visual_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR window_title[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        std::wstring title(window_title);
        if (title.length() >= 13 && title.substr(title.length() - 13) == L"Visual Studio") {
            taskbar.visual_windows++;
        }
    }
    return TRUE;
}

BOOL CALLBACK enum_firefox_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR window_title[buffer_size];
    WCHAR className[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        GetClassNameW(hwnd, className, buffer_size);
        std::wstring wTitle(window_title);
        std::wstring wClass(className);
        if (wClass == L"MozillaWindowClass" && wTitle != L"Mozilla Firefox Private Browsing") {
            taskbar.firefox_windows++;
        }
    }
    return TRUE;
}

void Taskbar::switch_windows(int keycode) {
    if (switch_keycode == keycode) {
        ac::keyboard::send_number_to_winkey(switch_position);
        auto_core.logg_and_logg("cycle window");
    }
    else {
        ac::keyboard::release_winkey();
        auto_core.logg_and_logg("window selected");
        switch_set = false;
        if (winkey_locked) {
            auto_core.print("winkey unlocked");
            winkey_locked = false;
        }
    }
}

std::optional<int> get_taskbar_position(
    const std::string_view name
) {
    const auto position = taskbar.position(name);

    if (!position) {
        auto_core.print(
            "Taskbar position '{}' is not configured.",
            name
        );

        return std::nullopt;
    }

    if (*position < 0 || *position > 9) {
        auto_core.print(
            "Invalid taskbar position for '{}': {}. "
            "Expected 0 through 9.",
            name,
            *position
        );

        return std::nullopt;
    }

    return position;
}

void Taskbar::activate_position_single(const std::string_view name) {
    const auto position = get_taskbar_position(name);

    if (!position) {
        return;
    }

    ac::keyboard::send_winkey(*position);
}

void Taskbar::activate_position_multiple(
    const std::string_view name,
    int& window_count,
    const WNDENUMPROC enum_windows
) {
    const auto position = get_taskbar_position(name);

    if (!position) {
        return;
    }

    window_count = 0;

    if (!EnumWindows(
        enum_windows,
        reinterpret_cast<LPARAM>(&window_count)
    )) {
        auto_core.print(
            "Unable to enumerate windows for '{}'. Error: {}.",
            name,
            GetLastError()
        );

        return;
    }

    if (window_count < 2) {
        ac::keyboard::send_winkey(*position);
        return;
    }

    auto_core.logg_and_logg(
        "Multiple windows detected for '{}': {}",
        name,
        window_count
    );

    ac::keyboard::press_and_hold_winkey();

    switch_set = true;
    switch_position = *position;

    ac::keyboard::send_number_to_winkey(*position);
}

void Taskbar::activate_auto_core() {
    if (auto result = ac::console::activate(); !result) {
        activate_position_single("auto_core");
    }
}

void Taskbar::activate_folder() {
    activate_position_multiple(
        "folder",
        folder_windows,
        enum_folder_windows
    );
}

void Taskbar::activate_word() {
    activate_position_multiple(
        "word",
        word_windows,
        enum_word_windows
    );
}

void Taskbar::activate_vs_code() {
    activate_position_multiple(
        "vs_code",
        vs_code_windows,
        enum_vs_code_windows
    );
}

void Taskbar::activate_iTunes() {
    activate_position_single("itunes");
}

void Taskbar::activate_chrome() {
    activate_position_multiple(
        "chrome",
        chrome_windows,
        enum_chrome_windows
    );
}

void Taskbar::activate_visual() {
    activate_position_multiple(
        "visual",
        visual_windows,
        enum_visual_windows
    );
}

void Taskbar::activate_discord() {
    activate_position_single("discord");
}

void Taskbar::activate_firefox() {
    activate_position_multiple(
        "firefox",
        firefox_windows,
        enum_firefox_windows
    );
}

void Taskbar::activate_spotify() {
    activate_position_single("spotify");
}

/** \keymap_command */
void activate_auto_core() {
    auto_core.logg_and_logg("activate_auto_core()");
    taskbar.activate_auto_core();
}

/** \keymap_command */
void activate_folder() {
    auto_core.logg_and_logg("activate_folder()");
    taskbar.activate_folder();
}

/** \keymap_command */
void activate_word() {
    auto_core.logg_and_logg("activate_word()");
    taskbar.activate_word();
}

/** \keymap_command */
void activate_vs_code() {
    auto_core.logg_and_logg("activate_vs_code()");
    taskbar.activate_vs_code();
}

/** \keymap_command */
void activate_iTunes() {
    auto_core.logg_and_logg("activate_iTunes()");
    taskbar.activate_iTunes();
}

/** \keymap_command */
void activate_chrome() {
    auto_core.logg_and_logg("activate_chrome()");
    taskbar.activate_chrome();
}

/** \keymap_command */
void activate_visual() {
    auto_core.logg_and_logg("activate_visual()");
    taskbar.activate_visual();
}

/** \keymap_command */
void activate_discord() {
    auto_core.logg_and_logg("activate_discord()");
    taskbar.activate_discord();
}

bool is_firefox_window_class(const std::wstring_view class_name) {
    return class_name == L"MozillaWindowClass" ||
        class_name.starts_with(L"Mozilla_firefox_");
}

bool is_foreground_window_firefox() {
    const HWND hwnd = GetForegroundWindow();

    if (hwnd == nullptr) {
        return false;
    }

    std::array<wchar_t, 256> class_name {};

    const int class_length = GetClassNameW(
        hwnd,
        class_name.data(),
        static_cast<int>(class_name.size())
    );

    if (class_length <= 0) {
        return false;
    }

    return is_firefox_window_class(
        std::wstring_view {
            class_name.data(),
            static_cast<std::size_t>(class_length)
        }
    );
}

/** \keymap_command */
void refresh_firefox() {
    auto_core.logg_and_logg("refresh_firefox()");
    if (is_foreground_window_firefox()) {
        start_reddit_new_tab();
    }
    else {
        activate_firefox();
    }
}

/** \keymap_command */
void start_reddit_new_tab() {
    auto_core.logg_and_logg("start_reddit_new_tab()");
    std::wstring url = L"https://www.reddit.com";
    std::wstring firefox_path = LR"(C:\Program Files\Mozilla Firefox\firefox.exe)";
    ShellExecuteW(0, 0, firefox_path.c_str(), url.c_str(), 0, SW_SHOW);
}

/** \keymap_command */
void activate_firefox() {
    auto_core.logg_and_logg("activate_firefox()");
    taskbar.activate_firefox();
}

/** \keymap_command */
void activate_spotify() {
    auto_core.logg_and_logg("activate_spotify()");
    taskbar.activate_spotify();
}

void taskbar_runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("activate_auto_core", &::activate_auto_core);
    registry.add("activate_folder", &::activate_folder);
    registry.add("activate_word", &::activate_word);
    registry.add("activate_vs_code", &::activate_vs_code);
    registry.add("activate_iTunes", &::activate_iTunes);
    registry.add("activate_chrome", &::activate_chrome);
    registry.add("activate_visual", &::activate_visual);
    registry.add("activate_discord", &::activate_discord);
    registry.add("refresh_firefox", &::refresh_firefox);
    registry.add("start_reddit_new_tab", &::start_reddit_new_tab);
    registry.add("activate_firefox", &::activate_firefox);
    registry.add("activate_spotify", &::activate_spotify);
}
