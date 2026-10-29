module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

module taskbar_firefox;

import std;
import auto_core.main.application;
import auto_core.main.taskbar;

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

void open_reddit_in_firefox_new_tab() {
    auto_core.log_info("open_reddit_in_firefox_new_tab()");

    std::wstring url = L"https://www.reddit.com";
    std::wstring firefox_path =
        LR"(C:\Program Files\Mozilla Firefox\firefox.exe)";

    ShellExecuteW(
        nullptr,
        nullptr,
        firefox_path.c_str(),
        url.c_str(),
        nullptr,
        SW_SHOW
    );
}

void activate_firefox() {
    taskbar.activate_configured("firefox");
}

void activate_firefox_or_open_reddit() {
    auto_core.log_info("activate_firefox_or_open_reddit()");
    if (is_foreground_window_firefox()) {
        open_reddit_in_firefox_new_tab();
    }
    else {
        activate_firefox();
    }
}
