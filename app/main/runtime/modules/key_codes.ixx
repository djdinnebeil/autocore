/**
\file key_codes.ixx
\brief Defines normalized keyboard key codes and configuration-name resolution.

This module defines the keyboard inputs supported by Auto Core. Most values match
Windows virtual-key codes. Numpad Enter uses a normalized value outside the
virtual-key range so it remains distinct from the main Enter key.

\note Both Enter keys report virtual-key code 13. The keyboard hook uses the
LLKHF_EXTENDED flag to normalize Numpad Enter to its distinct value.
*/
export module auto_core.main.key_codes;

import std;

export namespace key_codes {
    using Code = int;

    inline constexpr Code enter = 13;
    inline constexpr Code numpad_0 = 96;
    inline constexpr Code numpad_1 = 97;
    inline constexpr Code numpad_2 = 98;
    inline constexpr Code numpad_3 = 99;
    inline constexpr Code numpad_4 = 100;
    inline constexpr Code numpad_5 = 101;
    inline constexpr Code numpad_6 = 102;
    inline constexpr Code numpad_7 = 103;
    inline constexpr Code numpad_8 = 104;
    inline constexpr Code numpad_9 = 105;
    inline constexpr Code numpad_multiply = 106;
    inline constexpr Code numpad_add = 107;
    inline constexpr Code numpad_subtract = 109;
    inline constexpr Code numpad_decimal = 110;
    inline constexpr Code numpad_divide = 111;
    inline constexpr Code browser_home = 172;
    inline constexpr Code media_play_pause = 179;
    inline constexpr Code launch_mail = 180;
    inline constexpr Code launch_calculator = 183;

    // Windows virtual-key codes occupy one byte. Keep normalized values above
    // that range to avoid collisions with real keys such as VK_SEPARATOR.
    inline constexpr Code numpad_enter = 0x100;

    /** A `keymap.map` key name and its normalized code. */
    struct NamedCode {
        std::string_view name;
        Code code;
    };

    /**
     * Names accepted by `resolve`, in seed-file order. `enter` is not a name;
     * only `numpad_enter` is distinct from the main Enter key.
     */
    inline constexpr std::array<NamedCode, 20> keys {{
        {"numpad_0", numpad_0},
        {"numpad_1", numpad_1},
        {"numpad_2", numpad_2},
        {"numpad_3", numpad_3},
        {"numpad_4", numpad_4},
        {"numpad_5", numpad_5},
        {"numpad_6", numpad_6},
        {"numpad_7", numpad_7},
        {"numpad_8", numpad_8},
        {"numpad_9", numpad_9},
        {"numpad_multiply", numpad_multiply},
        {"numpad_add", numpad_add},
        {"numpad_decimal", numpad_decimal},
        {"numpad_enter", numpad_enter},
        {"numpad_subtract", numpad_subtract},
        {"numpad_divide", numpad_divide},
        {"media_play_pause", media_play_pause},
        {"browser_home", browser_home},
        {"launch_mail", launch_mail},
        {"launch_calculator", launch_calculator},
    }};

    /**
     * Resolves a `keymap.map` key name. `enter` is not mapped; only
     * `numpad_enter` is distinct from the main Enter key.
     */
    [[nodiscard]]
    constexpr std::optional<Code> resolve(std::string_view name) {
        for (const auto& key : keys) {
            if (key.name == name) {
                return key.code;
            }
        }

        return std::nullopt;
    }
}
