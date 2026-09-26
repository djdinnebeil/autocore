module auto_core.main.application;

import std;
import auto_core.core.console;
import auto_core.core.paths;

import <Windows.h>;

import auto_core.main.components;

ac::Component auto_core {"auto_core"};

bool ac::main::create_process(
    const std::filesystem::path& executable_path,
    const std::wstring_view arguments,
    const std::uint32_t creation_flags
) {
    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info {};

    const std::filesystem::path working_directory =
        executable_path.parent_path();

    std::wstring command_line =
        L"\"" + executable_path.wstring() + L"\"";

    if (!arguments.empty()) {
        command_line += L' ';
        command_line += arguments;
    }

    if (!CreateProcessW(
        executable_path.c_str(),
        command_line.data(),
        nullptr,
        nullptr,
        FALSE,
        creation_flags,
        nullptr,
        working_directory.c_str(),
        &startup_info,
        &process_info
    )) {
        const DWORD error = GetLastError();

        auto_core.print(
            "Unable to start '{}'. GetLastError = {}",
            executable_path,
            error
        );

        return false;
    }

    (void)AllowSetForegroundWindow(process_info.dwProcessId);
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    return true;
}

namespace {
    std::unordered_set<HWND> visible_top_windows() {
        std::unordered_set<HWND> windows;
        EnumWindows(
            [](HWND window, LPARAM parameter) -> BOOL {
                auto& found = *reinterpret_cast<std::unordered_set<HWND>*>(
                    parameter
                );
                if (IsWindowVisible(window) &&
                    GetWindow(window, GW_OWNER) == nullptr) {
                    found.insert(window);
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&windows)
        );
        return windows;
    }
}

bool ac::main::create_process_and_focus(
    const std::filesystem::path& executable_path,
    const std::wstring_view arguments,
    const std::uint32_t creation_flags
) {
    const HWND our_console = GetConsoleWindow();
    const HWND previous_foreground = GetForegroundWindow();
    const auto existing = visible_top_windows();
    if (!create_process(executable_path, arguments, creation_flags)) {
        return false;
    }

    constexpr auto timeout = std::chrono::seconds {3};
    constexpr auto poll = std::chrono::milliseconds {50};
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        const HWND foreground = GetForegroundWindow();
        if (foreground != nullptr &&
            foreground != our_console &&
            foreground != previous_foreground) {
            (void)ac::console::activate_window(foreground);
            return true;
        }
        for (const HWND window : visible_top_windows()) {
            if (window != our_console && !existing.contains(window)) {
                (void)ac::console::activate_window(window);
                return true;
            }
        }
        std::this_thread::sleep_for(poll);
    }

    auto_core.log_main(
        "Started '{}' but could not confirm keyboard focus.",
        executable_path
    );
    return true;
}

/**
 * \brief Closes the program.
 *
 * Hides and detaches the console, stops running child components, unhooks
 * the keyboard hook, and posts a quit message to the main thread.
 *
 * \keymap_command
 */
namespace {

void close_program_impl(const bool allow_recovery_prompt) {
    auto_core.log_main("close_program()");
    ac::main::program_closing = true;
    if (ac::main::keyboard_hook != NULL) {
        UnhookWindowsHookEx(ac::main::keyboard_hook);
        ac::main::keyboard_hook = NULL;
    }
    const bool keep_console_visible =
        allow_recovery_prompt &&
        ac::main::components::console_shutdown_prompt_enabled();
    if (!keep_console_visible) {
        if (const HWND console = GetConsoleWindow(); console != nullptr) {
            ShowWindow(console, SW_HIDE);
        }
        (void)FreeConsole();
    }
    ac::main::components::shutdown(allow_recovery_prompt);
    if (keep_console_visible) {
        if (const HWND console = GetConsoleWindow(); console != nullptr) {
            ShowWindow(console, SW_HIDE);
        }
        (void)FreeConsole();
    }
    PostThreadMessage(ac::main::main_thread_id, WM_QUIT, 0, 0);
}

}

void close_program() {
    close_program_impl(true);
}

void close_program_noninteractive() {
    close_program_impl(false);
}

/**
    * \brief Activates the function key.
    *
    * This function sets the primary flag to false and logs the activation of the function key.
    *
    * \keymap_command
    */
void activate_function_key() {
    primary = false;
    auto_core.print("Function key activated");
}

/**
    * \brief Deactivates the function key.
    *
    * This function sets the primary flag to true and logs the deactivation of the function key.
    *
    * \keymap_command
    */
void deactivate_function_key() {
    primary = true;
    auto_core.print("Function key deactivated");
}

/**
    * \brief Sets focus to the Auto Core window.
    *
    * This function sets the focus to the Auto Core window, clearing the input buffer
    * and activating the Auto Core window if it is not already in focus.
    */
void set_focus_auto_core() {
    auto_core.log_main("set_focus_auto_core()");

    if (auto result = ac::console::focus_for_prompt(); !result) {
        auto_core.log_print(
            ac::console::error_message(result.error())
        );
    }
}

void main_component::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("close_program", &::close_program);
    registry.add("activate_function_key", &::activate_function_key);
    registry.add("deactivate_function_key", &::deactivate_function_key);
    registry.add("launch_journal_config", [] {
        const auto executable =
            ac::paths::bin_directory() / "journal_config.exe";
        if (!ac::main::create_process_and_focus(
                executable, {}, CREATE_NEW_CONSOLE
            )) {
            auto_core.log_print(
                "Unable to start journal_config.exe."
            );
        }
    });
}
