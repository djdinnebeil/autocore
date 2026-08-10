/**
 * \file end.ixx
 * \brief Handles system shutdown events.
 *
 * This module monitors a hidden window to detect and respond to system shutdown events.
 * It handles various console close events and system end session messages, ensuring
 * that the program can respond appropriately to system shutdown requests.
 */
export module end;

import <Windows.h>;

export {
    BOOL WINAPI console_close_event(DWORD dwType);
    HWND close_window_hidden_init();
}
