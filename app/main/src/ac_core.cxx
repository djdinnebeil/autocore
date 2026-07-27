module ac_core;
import visual;
import <Windows.h>;
import <conio.h>;
import sp;
import itunes;
import logger_x;
import server;
import taskbar;
import taskbar_ps;


/**
    * \brief Closes the program.
    *
    * This function performs the necessary cleanup and shutdown operations for the program.
    * It stops the server, sends end signals to iTunes and Spotify, destroys the taskbar window,
    * unhooks the keyboard hook, and posts a quit message to the main thread.
    *
    * \runtime
    */
void close_program() {
    ac::logger::logg("close_program()");
    ac::main::program_closing = true;
    stop_server();
    send_iTunes_end_signal();
    send_sp_end_signal();
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
    * \runtime
    */
void activate_function_key() {
    primary = false;
    ac::print("Function key activated");
}

/**
    * \brief Deactivates the function key.
    *
    * This function sets the primary flag to true and logs the deactivation of the function key.
    *
    * \runtime
    */
void deactivate_function_key() {
    primary = true;
    ac::print("Function key deactivated");
}


/**
    * \brief Clears the input buffer.
    *
    * This function clears any remaining input in the input buffer.
    */
void clear_input_buffer() {
    while (_kbhit()) {
        int getch = _getch();
    }
    std::cin.clear();
}

/**
    * \brief Sets focus to the Auto Core window.
    *
    * This function sets the focus to the Auto Core window, clearing the input buffer
    * and activating the Auto Core window if it is not already in focus.
    */
void set_focus_auto_core() {
    ac::logger::logg("set_focus_auto_core()");
    clear_input_buffer();
    HWND current_window_hwnd = GetForegroundWindow();
    if (current_window_hwnd != ac::main::program_window) {
        taskbar.activate_auto_core();
    }
}
