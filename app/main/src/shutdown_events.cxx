module auto_core.main.shutdown_events;

import std;
import auto_core.main.application;

import <Windows.h>;

namespace {

inline constexpr UINT shutdown_request_message = WM_APP + 97;

void request_shutdown() {
    PostThreadMessage(
        ac::main::main_thread_id,
        shutdown_request_message,
        0,
        0
    );
}

/**
 * \brief Window procedure for handling close events.
 *
 * This function processes messages sent to the hidden window. It handles
 * WM_QUERYENDSESSION and WM_ENDSESSION messages to detect system shutdown events.
 *
 * \param window Handle to the window.
 * \param message The message code.
 * \param w_param Additional message information.
 * \param l_param Additional message information.
 * \return The result of the message processing.
 */
LRESULT CALLBACK shutdown_window_procedure(
    HWND window,
    UINT message,
    WPARAM w_param,
    LPARAM l_param
) {
    switch (message) {
    case WM_QUERYENDSESSION:
        auto_core.logg_and_logg(
            "shutdown_window_procedure() - WM_QUERYENDSESSION"
        );
        return TRUE;
    case WM_ENDSESSION:
        if (w_param == TRUE) {
            auto_core.logg_and_logg(
                "shutdown_window_procedure() - WM_ENDSESSION"
            );
            request_shutdown();
            return TRUE;
        }
        break;
    default:
        return DefWindowProc(window, message, w_param, l_param);
    }
    return 0;
}

/**
 * \brief Initializes a hidden window to detect close events.
 *
 * This function creates a hidden window with a custom window procedure to
 * handle system shutdown events.
 *
 * \return Handle to the created hidden window.
 */
HWND create_shutdown_window() {
    const wchar_t CLASS_NAME[] = L"Auto Core Close Window Hidden Class";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = shutdown_window_procedure;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);
    return CreateWindowExW(
        0,
        CLASS_NAME,
        L"Auto Core Close Window Hidden",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        wc.hInstance,
        NULL
    );
}


/**
 * \brief Handles console close events.
 *
 * This function processes console close events such as CTRL_CLOSE_EVENT,
 * CTRL_BREAK_EVENT, and CTRL_C_EVENT. It ensures that the program can respond
 * appropriately by logging the event and posting a message to the main thread.
 *
 * \param control_type The type of control signal received.
 * \return TRUE if the event was handled, FALSE otherwise.
 */
BOOL WINAPI console_control_handler(DWORD control_type) {
    if (!ac::main::program_closing) {
        switch (control_type) {
        case CTRL_CLOSE_EVENT:
            auto_core.logg_and_logg(
                "console_control_handler() - CTRL_CLOSE_EVENT"
            );
            request_shutdown();
            return TRUE;
        case CTRL_BREAK_EVENT:
            auto_core.logg_and_logg(
                "console_control_handler() - CTRL_BREAK_EVENT"
            );
            request_shutdown();
            return TRUE;
        case CTRL_C_EVENT:
            auto_core.logg_and_logg(
                "console_control_handler() - CTRL_C_EVENT"
            );
            request_shutdown();
            return TRUE;
        default:
            auto_core.logg_and_logg(
                "console_control_handler() - default"
            );
            break;
        }
    }
    return FALSE;
}

}

bool initialize_shutdown_events() {
    if (!SetConsoleCtrlHandler(console_control_handler, TRUE)) {
        return false;
    }

    if (create_shutdown_window() != nullptr) {
        return true;
    }

    SetConsoleCtrlHandler(console_control_handler, FALSE);
    return false;
}

bool process_shutdown_event(const MSG& message) {
    if (message.message != shutdown_request_message) {
        return false;
    }

    close_program();
    return true;
}
