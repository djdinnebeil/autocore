module auto_core.core.keyboard;

import std;
import auto_core.core.error;
import <Windows.h>;

namespace ac::keyboard {

    namespace {

        constexpr std::size_t stack_input_capacity = 8;

        std::string format_key_values(
            const std::span<const std::uint8_t> keys
        ) {
            std::string result;

            for (const std::uint8_t key : keys) {
                if (!result.empty()) {
                    result += ", ";
                }

                result += std::format(
                    "0x{:02X}",
                    static_cast<unsigned int>(key)
                );
            }

            return result;
        }

        bool send_keyboard_inputs(
            INPUT* inputs,
            const UINT count,
            const std::string& what
        ) {
            SetLastError(ERROR_SUCCESS);

            const UINT sent = SendInput(count, inputs, sizeof(INPUT));
            if (sent == count) {
                return true;
            }

            ac::error::log(
                "Unable to send {}. Sent {} of {} input events. Error: {}.\n",
                what,
                sent,
                count,
                GetLastError()
            );
            return false;
        }

        void send_single_key(const BYTE vk_code) {
            INPUT inputs[2] = {};

            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = vk_code;

            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = vk_code;
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

            send_keyboard_inputs(
                inputs,
                ARRAYSIZE(inputs),
                "single key"
            );
        }

    }

    bool send_winkey(const int position) {
        if (position < 0 || position > 9) {
            ac::error::log(
                "Invalid taskbar position: {}. Expected 0 through 9.\n",
                position
            );

            return false;
        }

        const std::array<BYTE, 2> keys {
            VK_RWIN,
            static_cast<BYTE>('0' + position)
        };

        return send_key_combination(keys);
    }

    bool send_taskbar_position(const std::uint8_t position) {
        if (position < 1 || position > 10) {
            ac::error::log(
                "Invalid logical taskbar position: {}. Expected 1 through 10.\n",
                position
            );
            return false;
        }

        return send_winkey(position == 10 ? 0 : position);
    }

    void send_number_to_winkey(const int position) {
        if (position < 0 || position > 9) {
            ac::error::log(
                "Invalid taskbar position: {}. Expected 0 through 9.\n",
                position
            );
            return;
        }
        send_single_key(static_cast<BYTE>('0' + position));
    }

    void send_taskbar_position_while_win_held(
        const std::uint8_t position
    ) {
        if (position < 1 || position > 10) {
            ac::error::log(
                "Invalid logical taskbar position: {}. Expected 1 through 10.\n",
                position
            );
            return;
        }

        send_number_to_winkey(position == 10 ? 0 : position);
    }

    void press_and_hold_winkey() {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_RWIN;

        send_keyboard_inputs(&input, 1, "Windows-key press");
    }

    void release_winkey() {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_RWIN;
        input.ki.dwFlags = KEYEVENTF_KEYUP;

        send_keyboard_inputs(&input, 1, "Windows-key release");
    }

    bool send_linebreak() {
        constexpr std::array<BYTE, 2> keys {
            VK_SHIFT,
            VK_RETURN
        };

        return send_key_combination(keys);
    }

    [[nodiscard]] bool send_key_combination(
        std::span<const std::uint8_t> keys
    ) {
        if (keys.empty()) {
            ac::error::log(
                "Unable to send key combination: "
                "no key values were provided.\n"
            );

            return false;
        }

        const std::size_t key_count = keys.size();

        if (key_count > UINT_MAX / 2) {
            ac::error::log(
                "Unable to send key combination [{}]: "
                "too many key values were provided.\n",
                format_key_values(keys)
            );

            return false;
        }

        const std::size_t total_inputs = key_count * 2;
        const UINT input_count =
            static_cast<UINT>(total_inputs);

        INPUT stack_inputs[stack_input_capacity] {};
        std::vector<INPUT> heap_inputs;
        INPUT* inputs = stack_inputs;

        if (total_inputs > stack_input_capacity) {
            heap_inputs.assign(total_inputs, INPUT{});
            inputs = heap_inputs.data();
        }

        for (std::size_t i = 0; i < key_count; ++i) {
            inputs[i].type = INPUT_KEYBOARD;
            inputs[i].ki.wVk =
                static_cast<WORD>(keys[i]);

            inputs[i + key_count].type = INPUT_KEYBOARD;
            inputs[i + key_count].ki.wVk =
                static_cast<WORD>(keys[key_count - 1 - i]);

            inputs[i + key_count].ki.dwFlags =
                KEYEVENTF_KEYUP;
        }

        return send_keyboard_inputs(
            inputs,
            input_count,
            std::format(
                "key combination [{}]",
                format_key_values(keys)
            )
        );
    }

}
