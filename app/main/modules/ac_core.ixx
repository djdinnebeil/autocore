/**
 * \file ac_core.ixx
 * \brief Core support for cross-module communication and integration within the Auto Core framework.
 *
 * This module provides core functionality for managing program state and facilitating
 * communication between different modules within the Auto Core framework. It includes
 * functions for closing the program, activating and deactivating function keys, printing
 * timestamps, and setting focus to the Auto Core window.
 */
export module ac_core;
import visual;
import <Windows.h>;
import <conio.h>;
import sp;
import itunes;
import logger_x;
import server;
import taskbar;
import taskbar_ps;

export namespace ac::main {
    bool program_closing = false;
    HWND program_window;
    HWND close_window;
    HHOOK keyboard_hook;
    DWORD main_thread_id;
}

export bool primary = true;

export {
    void close_program();
    void activate_function_key();
    void deactivate_function_key();
    void set_focus_auto_core();
}
