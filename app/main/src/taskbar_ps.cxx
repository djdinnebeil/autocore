module taskbar_ps;

import std;
import auto_core.keyboard;
import ac_main;

import <Windows.h>;

HWND taskbar_ps_hwnd;

namespace {
    void send_ctrl_alt(const BYTE key) {
        const BYTE keys[] = {
            VK_CONTROL,
            VK_MENU,
            key
        };

        if (!ac::keyboard::send_key_combination(keys)) {
            auto_core.print("Invalid key combination for Ctrl+Alt+value");
       }
    }
}

void send_ctrl_alt_n() {
    send_ctrl_alt('N');
}

void send_ctrl_alt_p() {
    send_ctrl_alt('P');
}

void send_ctrl_alt_r() {
    send_ctrl_alt('R');
}

void send_ctrl_alt_x() {
    send_ctrl_alt('X');
}

void send_ctrl_alt_t() {
    send_ctrl_alt('T');
}

void send_ctrl_alt_l() {
    send_ctrl_alt('L');
}

void launch_notepad() {
    ShellExecuteW(NULL, L"open", L"notepad.exe", NULL, NULL, SW_SHOWDEFAULT);
    auto_core.print("Launching Notepad");
}

void launch_powershell() {
    ShellExecuteW(NULL, L"open", L"powershell.exe", NULL, NULL, SW_SHOWDEFAULT);
    auto_core.print("Launching PowerShell");
}

void launch_ps_in_visual_key() {
    ShellExecuteW(NULL, L"open", LR"(C:\DJ\My Folder\Visual Key\powershell.exe.lnk)", NULL, NULL, SW_SHOWDEFAULT);
    auto_core.print("Launching PowerShell in Visual Key");
}

void launch_webstorm() {
    ShellExecuteW(NULL, L"open", LR"(C:\DJ\My Folder\Visual Key\WebStorm.lnk)", NULL, NULL, SW_SHOWDEFAULT);
    auto_core.print("Launching WebStorm");
}

void launch_gitbash() {
    ShellExecuteW(NULL, L"open", LR"(C:\DJ\My Folder\Visual Key\gitbash.exe.lnk)", NULL, NULL, SW_SHOWDEFAULT);
    auto_core.print("Launching Git Bash");
}

void launch_wordpad() {
    ShellExecuteW(NULL, L"open", L"wordpad.exe", NULL, NULL, SW_SHOWDEFAULT);
    auto_core.print("Launching WordPad");
}

LRESULT CALLBACK register_hotkey_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        if (!RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_ALT, 'N') ||
            !RegisterHotKey(hwnd, 2, MOD_CONTROL | MOD_ALT, 'P') ||
            !RegisterHotKey(hwnd, 3, MOD_CONTROL | MOD_ALT, 'R') ||
            !RegisterHotKey(hwnd, 4, MOD_CONTROL | MOD_ALT, 'X') ||
            !RegisterHotKey(hwnd, 5, MOD_CONTROL | MOD_ALT, 'T') ||
            !RegisterHotKey(hwnd, 6, MOD_CONTROL | MOD_ALT, 'L')) {
            auto_core.print("Failed to register one or more hotkeys.");
        }
        else {
            auto_core.logg_and_logg("All hotkeys registered successfully.");
        }
        break;
    case WM_HOTKEY:
    {
        std::string hotkey_name;
        switch (wParam) {
        case 1: hotkey_name = "Notepad"; break;
        case 2: hotkey_name = "PowerShell"; break;
        case 3: hotkey_name = "Gitbash"; break;
        case 4: hotkey_name = "WordPad"; break;
        case 5: hotkey_name = "PowerShellX"; break;
        case 6: hotkey_name = "WebStorm"; break;

        default: hotkey_name = "Unknown"; break;
        }
        auto_core.logg_and_logg("{} hotkey triggered", hotkey_name);
        switch (wParam) {
        case 1: launch_notepad(); break;
        case 2: launch_powershell(); break;
        case 3: launch_gitbash(); break;
        case 4: launch_wordpad(); break;
        case 5: launch_ps_in_visual_key(); break;
        case 6: launch_webstorm(); break;
        }
    }
    break;
    case WM_DESTROY:
        UnregisterHotKey(hwnd, 1);
        UnregisterHotKey(hwnd, 2);
        UnregisterHotKey(hwnd, 3);
        UnregisterHotKey(hwnd, 4);
        UnregisterHotKey(hwnd, 5);
        UnregisterHotKey(hwnd, 6);
        auto_core.logg_and_logg("All hotkeys unregistered.");
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void run_taskbar_ps() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = register_hotkey_proc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"TaskbarPSClass";

    if (!RegisterClassW(&wc)) {
        auto_core.print("Failed to register window class.");
        return;
    }

    taskbar_ps_hwnd = CreateWindowExW(
        0, L"TaskbarPSClass", L"Hidden Hotkey Window", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, wc.hInstance, NULL
    );

    if (!taskbar_ps_hwnd) {
        auto_core.print("Failed to create the hotkey window.");
        return;
    }

    ShowWindow(taskbar_ps_hwnd, SW_HIDE);
    auto_core.logg_and_logg("Hidden hotkey window created and shown.");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    auto_core.logg_and_logg("Message loop exited.");
}
