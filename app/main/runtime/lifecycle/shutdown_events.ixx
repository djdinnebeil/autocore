/**
 * \file shutdown_events.ixx
 * \brief Handles console and Windows session shutdown events.
 *
 * A hidden window receives `WM_QUERYENDSESSION` and `WM_ENDSESSION`. Console
 * close events are forwarded to the same shutdown path.
 */
export module auto_core.main.shutdown_events;

import <Windows.h>;

export {
    /** Installs the hidden window and console control handler. */
    bool initialize_shutdown_events();
    /**
     * \brief Consumes a posted shutdown request from the message loop.
     * \return `true` when `message` was a shutdown request.
     */
    bool process_shutdown_event(const MSG& message);
}
