/**
 * \file keyboard.ixx
 * \brief Supports the simulation of keyboard events.
 */
module;

#include "ac_api.hpp"

export module auto_core.keyboard;

import std;

export namespace ac::keyboard {
    /**
     * \brief Presses and releases the Windows key with a taskbar position.
     * \param position A taskbar position from 0 through 9.
     *
     * Invalid positions and input failures are reported through the error
     * logger. Position 0 sends Win+0.
     */
    AC_API void send_winkey(int position);
    /**
     * \brief Presses and releases a number key while Win remains held.
     * \param number A number from 0 through 9.
     *
     * This is intended for use between `press_and_hold_winkey()` and
     * `release_winkey()`. Invalid numbers are reported to stderr.
     */
    AC_API void send_number_to_winkey(int number);
    /** \brief Sends a Windows-key press without a matching release. */
    AC_API void press_and_hold_winkey();
    /** \brief Sends a Windows-key release. */
    AC_API void release_winkey();
    /**
     * \brief Presses keys in order and releases them in reverse order.
     * \param keys Win32 virtual-key codes.
     * \return `true` when Windows accepts every generated input event.
     *
     * An empty or excessively large combination returns `false`. Failures are
     * reported to stderr with the key values and Win32 error code.
     */
    AC_API [[nodiscard]]
        bool send_key_combination(std::span<const std::uint8_t> keys);
} // namespace ac::keyboard
