/**
 * \file keyboard.ixx
 * \brief Supports the simulation of keyboard events.
 */
module;

#include "ac_api.hpp"

export module keyboard;

import std;

export namespace ac::keyboard {
    AC_API void send_winkey(int position);
    AC_API void send_number_to_winkey(int number);
    AC_API void press_and_hold_winkey();
    AC_API void release_winkey();
    AC_API [[nodiscard]]
        bool send_key_combination(std::span<const std::uint8_t> keys);
}