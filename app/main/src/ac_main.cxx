module ac_main;

import std;
import auto_core.console;

import <Windows.h>;

import sp;
import itunes;
import journal;
import wake;
import server;
import taskbar_ps;

ac::Component auto_core {"auto_core"};

/**
    * \brief Closes the program.
    *
    * This function performs the necessary cleanup and shutdown operations for the program.
    * It stops the server, sends end signals to iTunes and Spotify, destroys the taskbar window,
    * unhooks the keyboard hook, and posts a quit message to the main thread.
    *
    * \keymap_command
    */
void close_program() {
    auto_core.logg_and_logg("close_program()");
    ac::main::program_closing = true;
    stop_server();
    send_iTunes_end_signal();
    send_sp_end_signal();
    send_journal_end_signal();
    send_wake_end_signal();
    if (taskbar_ps_hwnd != NULL) {
        DestroyWindow(taskbar_ps_hwnd);
    }
    if (ac::main::keyboard_hook != NULL) {
        UnhookWindowsHookEx(ac::main::keyboard_hook);
    }
    PostThreadMessage(ac::main::main_thread_id, WM_QUIT, 0, 0);
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
    auto_core.logg_and_logg("set_focus_auto_core()");

    if (auto result = ac::console::focus_for_prompt(); !result) {
        auto_core.logg_and_print(
            ac::console::error_message(result.error())
        );
    }
}

void ac_main::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("close_program", &::close_program);
    registry.add("activate_function_key", &::activate_function_key);
    registry.add("deactivate_function_key", &::deactivate_function_key);
}
