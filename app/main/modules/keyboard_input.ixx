/**
 * \file keyboard_input.ixx
 * \brief Captures configured keyboard inputs and dispatches them on the main thread.
 */
export module auto_core.main.keyboard_input;

import <Windows.h>;

export namespace keyboard_input {
    /** Posted to the main thread; `lParam` is the normalized key code. */
    inline constexpr UINT key_event_message = WM_APP + 96;

    /**
     * Low-level keyboard hook. Must return quickly: it posts to the main
     * thread instead of running keymap actions. Only `WM_KEYDOWN` is
     * considered. Mapped keys, including auto-repeat, are swallowed.
     */
    LRESULT CALLBACK hook_callback(
        int hook_code,
        WPARAM message,
        LPARAM event_data
    );

    /**
     * Dispatches a posted key event on the main thread. Reads
     * `taskbar.switch_set` for interactive cycling (see docs/TODO.md).
     */
    bool process_key_event(const MSG& message);
}
