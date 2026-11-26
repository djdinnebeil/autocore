/**
 * \file keyboard_input.ixx
 * \brief Captures configured keyboard inputs and dispatches them on the main thread.
 */
export module auto_core.main.keyboard_input;

import <Windows.h>;

export namespace keyboard_input {
    /** Posted to the main thread; `lParam` is the normalized key code. */
    inline constexpr UINT key_event_message = WM_APP + 96;
    /** `wParam` values carried by `key_event_message`. */
    inline constexpr WPARAM dispatch_binding = 0;
    inline constexpr WPARAM select_cycle = 1;
    inline constexpr WPARAM release_and_replay = 2;

    /**
     * Low-level keyboard hook. Must return quickly: it posts to the main
     * thread instead of running keymap actions. Mapped keys, including
     * auto-repeat, are swallowed. A physical row 0 or numpad 0 that
     * selects a native taskbar cycle is swallowed through its repeats and
     * key-up, so that press does not also dispatch the configured 0
     * binding. Injected input is passed on, including the synthetic Win
     * chord and the row 0 used for taskbar slot 10. Injected
     * `VK_MEDIA_PLAY_PAUSE`, `VK_BROWSER_HOME`, `VK_LAUNCH_MAIL`, and
     * `VK_LAUNCH_APP2` follow the same path as physical keys.
     */
    LRESULT CALLBACK hook_callback(
        int hook_code,
        WPARAM message,
        LPARAM event_data
    );

    /**
     * Dispatches a posted key event on the main thread.
     */
    bool process_key_event(const MSG& message);
}
