/**
 * \file keyboard.ixx
 * \brief Supports the simulation of keyboard events.
 *
 * Win+digit chords use the right Windows key (`VK_RWIN`). Digit positions
 * are 0 through 9; logical taskbar positions are 1 through 10, where 10
 * sends Win+0. A hold session has no timeout; interactive cycling state
 * lives in Main, not in this DLL.
 *
 * Invalid arguments and `SendInput` failures are reported through
 * `ac::error`. Hold, release, and held-Win digit helpers stay `void`: a
 * failed send is logged and the caller still owns pairing press with
 * release.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.keyboard;

import std;

export namespace ac::keyboard {
    /**
     * \brief Presses and releases the Windows key with a number key.
     * \param position A Win+number digit from 0 through 9, not a logical
     * slot 1–10. Use `send_taskbar_position()` for logical positions.
     *
     * Invalid positions and input failures are reported through the error
     * logger. Position 0 sends Win+0.
     */
    AC_API [[nodiscard]] bool send_winkey(int position);
    /**
     * \brief Presses and releases Win with a logical taskbar position.
     * \param position A position from 1 through 10. Position 10 sends Win+0.
     */
    AC_API [[nodiscard]] bool send_taskbar_position(
        std::uint8_t position
    );
    /**
     * \brief Presses and releases a number key while Win remains held.
     * \param number A number from 0 through 9.
     *
     * This is intended for use between `press_and_hold_winkey()` and
     * `release_winkey()`. Invalid numbers and `SendInput` failures are
     * reported through `ac::error`.
     */
    AC_API void send_number_to_winkey(int number);
    /**
     * \brief Sends a logical taskbar position while Win remains held.
     * \param position A position from 1 through 10. Position 10 sends key 0.
     */
    AC_API void send_taskbar_position_while_win_held(
        std::uint8_t position
    );
    /**
     * \brief Sends a Windows-key press without a matching release.
     *
     * The caller owns the hold session. There is no timeout. Pair every
     * press with `release_winkey()`, including when `SendInput` fails and
     * is reported through `ac::error`. Interactive cycling state lives in
     * Main, not in this DLL.
     */
    AC_API void press_and_hold_winkey();
    /**
     * \brief Sends a Windows-key release for a hold started by
     * `press_and_hold_winkey()`.
     *
     * A failed `SendInput` is reported through `ac::error`. The caller
     * still owns the hold session.
     */
    AC_API void release_winkey();
    /**
     * \brief Sends Shift+Enter to insert a linebreak without submitting
     * common chat-style text fields.
     * \return `true` when Windows accepts every generated input event.
     */
    AC_API [[nodiscard]] bool send_linebreak();
    /**
     * \brief Presses keys in order and releases them in reverse order.
     * \param keys Win32 virtual-key codes.
     * \return `true` when Windows accepts every generated input event.
     *
     * An empty or excessively large combination returns `false`. Failures are
     * reported through `ac::error` with the key values and Win32 error code.
     */
    AC_API [[nodiscard]]
        bool send_key_combination(std::span<const std::uint8_t> keys);
} // namespace ac::keyboard
