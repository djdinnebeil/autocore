module taskbar_11;

import std;
import ac_main;
import auto_core.console;

import taskbar_ps;

import <Windows.h>;

int wordpad_windows;
HWND wordpad_hwnd;
int notepad_windows;
HWND notepad_hwnd;
int powershell_windows;
HWND powershell_hwnd;
int gitbash_windows;
HWND gitbash_hwnd;
int ps_in_visual_key_windows;
HWND ps_in_visual_key_hwnd;
int webstorm_windows;
HWND webstorm_hwnd;
int zoom_windows;
HWND zoom_window_hwnd;

BOOL CALLBACK enum_zoom_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR window_title[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        std::wstring title(window_title);
        if (title.length() >= 12 && title.starts_with(L"Zoom Meeting")) {
            zoom_window_hwnd = hwnd;
            zoom_windows++;
        }
    }
    return TRUE;
}

void taskbar_11::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("activate_wordpad", &::activate_wordpad);
    registry.add("activate_notepad", &::activate_notepad);
    registry.add("activate_gitbash", &::activate_gitbash);
    registry.add("activate_powershell", &::activate_powershell);
    registry.add("activate_ps_in_visual_key", &::activate_ps_in_visual_key);
    registry.add("activate_webstorm", &::activate_webstorm);
    registry.add("activate_zoom", &::activate_zoom);
}

BOOL CALLBACK enum_webstorm_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR  window_title[buffer_size];
    WCHAR  className[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        GetClassNameW(hwnd, className, buffer_size);
        std::wstring wClass(className);
        if (wClass == L"SunAwtFrame") {
            webstorm_hwnd = hwnd;
            webstorm_windows++;
        }
    }
    return TRUE;
}

BOOL CALLBACK enum_wordpad_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR  window_title[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        std::wstring title(window_title);
        if (title.length() >= 7 && title.substr(title.length() - 7) == L"WordPad") {
            wordpad_hwnd = hwnd;
            wordpad_windows++;
        }
    }
    return TRUE;
}

BOOL CALLBACK enum_notepad_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR window_title[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        std::wstring title(window_title);
        if (title.length() >= 7 && title.substr(title.length() - 7) == L"Notepad") {
            notepad_hwnd = hwnd;
            notepad_windows++;
        }
    }
    return TRUE;
}

BOOL CALLBACK enum_gitbash_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR window_title[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        std::wstring title(window_title);
        if (title.starts_with(L"/")) {
            gitbash_hwnd = hwnd;
            gitbash_windows++;
        }
    }
    return TRUE;
}

BOOL CALLBACK enum_powershell_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR window_title[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        std::wstring title(window_title);
        if (title.starts_with(L"Administrator:")) {
            powershell_hwnd = hwnd;
            powershell_windows++;
        }
    }
    return TRUE;
}

BOOL CALLBACK enum_ps_in_visual_key_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR window_title[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        std::wstring title(window_title);
        if (title.starts_with(L"DJ@DESKTOP:")) {
            ps_in_visual_key_hwnd = hwnd;
            ps_in_visual_key_windows++;
        }
    }
    return TRUE;
}

namespace {

    template<typename LaunchAction>
    void activate_detected_window(
        const std::string_view name,
        int& window_count,
        HWND& target_window,
        const WNDENUMPROC enumerate,
        LaunchAction&& launch
    ) {
        window_count = 0;
        target_window = nullptr;
        (void)EnumWindows(enumerate, 0);

        if (window_count == 0 || target_window == nullptr) {
            auto_core.logg_and_logg(
                "activate_{}() - launch new",
                name
            );
            std::invoke(std::forward<LaunchAction>(launch));
            return;
        }

        if (
            GetForegroundWindow() == target_window &&
            !IsIconic(target_window)
        ) {
            (void)ShowWindow(target_window, SW_MINIMIZE);
            auto_core.logg_and_logg(
                "activate_{}() - minimize",
                name
            );
            return;
        }

        const bool was_minimized = IsIconic(target_window) != FALSE;
        const auto result = ac::console::activate_window(target_window);

        if (!result) {
            auto_core.logg_and_print(
                "Unable to activate {}: {}",
                name,
                ac::console::error_message(result.error())
            );
            return;
        }

        if (window_count > 1) {
            auto_core.logg_and_logg(
                "activate_{}() - multiple windows detected: {}",
                name,
                window_count
            );
        }
        else if (was_minimized) {
            auto_core.logg_and_logg(
                "activate_{}() - restore",
                name
            );
        }
        else {
            auto_core.logg_and_logg(
                "activate_{}() - set focus",
                name
            );
        }
    }

}

/** \keymap_command */
void activate_wordpad() {
    activate_detected_window(
        "wordpad",
        wordpad_windows,
        wordpad_hwnd,
        enum_wordpad_windows,
        &send_ctrl_alt_x
    );
}

/** \keymap_command */
void activate_notepad() {
    activate_detected_window(
        "notepad",
        notepad_windows,
        notepad_hwnd,
        enum_notepad_windows,
        &send_ctrl_alt_n
    );
}

/** \keymap_command */
void activate_gitbash() {
    activate_detected_window(
        "gitbash",
        gitbash_windows,
        gitbash_hwnd,
        enum_gitbash_windows,
        &send_ctrl_alt_r
    );
}

/** \keymap_command */
void activate_powershell() {
    activate_detected_window(
        "powershell",
        powershell_windows,
        powershell_hwnd,
        enum_powershell_windows,
        &send_ctrl_alt_p
    );
}

/** \keymap_command */
void activate_ps_in_visual_key() {
    activate_detected_window(
        "ps_in_visual_key",
        ps_in_visual_key_windows,
        ps_in_visual_key_hwnd,
        enum_ps_in_visual_key_windows,
        &send_ctrl_alt_t
    );
}

/** \keymap_command */
void activate_webstorm() {
    activate_detected_window(
        "webstorm",
        webstorm_windows,
        webstorm_hwnd,
        enum_webstorm_windows,
        &send_ctrl_alt_l
    );
}

/** \keymap_command */
void activate_zoom() {
    activate_detected_window(
        "zoom",
        zoom_windows,
        zoom_window_hwnd,
        enum_zoom_windows,
        []() {
            auto_core.print(
                "activate_zoom() - launch new - not implemented"
            );
        }
    );
}
