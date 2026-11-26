/**
 * \file keyboard.ixx
 * \brief Supports the simulation of keyboard events.
 *
 * Win+digit chords use the right Windows key (`VK_RWIN`). Digit positions
 * are 0 through 9; logical taskbar positions are 1 through 10, where 10
 * sends Win+0.
 *
 * Invalid arguments and `SendInput` failures are reported through
 * `ac::error`.
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
     * \brief Presses the right Windows key and leaves it down.
     * \return `false` when `SendInput` does not accept the event.
     */
    AC_API [[nodiscard]] bool press_winkey();
    /**
     * \brief Releases the right Windows key pressed by `press_winkey`.
     * \return `false` when `SendInput` does not accept the event.
     */
    AC_API [[nodiscard]] bool release_winkey();
    /**
     * \brief Taps a taskbar digit without pressing or releasing Win.
     * \param digit A digit from 0 through 9. Digit 0 is the slot 10 key.
     * \return `false` when the digit is outside 0 through 9 or `SendInput`
     * does not accept both events.
     */
    AC_API [[nodiscard]] bool tap_taskbar_digit(std::uint8_t digit);
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
