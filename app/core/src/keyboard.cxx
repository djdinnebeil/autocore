module keyboard;

import std;
import error;
import <Windows.h>;

namespace ac::keyboard {

    namespace {

        /**
         * \brief Sends a single key press and release.
         * \param vk_code Virtual-key code of the key to press and release.
         */
        void send_single_key(const BYTE vk_code) {
            INPUT inputs[2] = {};

            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = vk_code;

            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = vk_code;
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

            SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        }

    }

    std::string format_key_values(
        const std::span<const BYTE> keys
    ) {
        std::string result;

        for (const BYTE key : keys) {
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

    /**
     * \brief Presses and releases the Windows key with a number key.
     * \param position Taskbar position represented by a number from 0 to 9.
     */
    void send_winkey(const int position) {
        if (position < 0 || position > 9) {
            ac::error::log(
                "Invalid taskbar position: {}. Expected 0 through 9.\n",
                position
            );

            return;
        }

        const std::array<BYTE, 2> keys {
            VK_RWIN,
            static_cast<BYTE>('0' + position)
        };

        if (!send_key_combination(keys)) {
            ac::error::log(
                "Unable to send the taskbar shortcut for position {}.\n",
                position
            );

        }
    }

    /**
     * \brief Sends a number key while preserving the Windows-key state.
     * \param number The number key to press and release (0-9).
     *
     * Intended for use after press_and_hold_winkey().
     */
    void send_number_to_winkey(const int position) {
        if (position < 0 || position > 9) {
            std::cerr
                << "Invalid taskbar position: "
                << position
                << ". Expected 0 through 9.\n";

            return;
        }
        send_single_key(static_cast<BYTE>('0' + position));
    }

    /**
     * \brief Presses and holds the Windows key.
     */
    void press_and_hold_winkey() {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_RWIN;

        SendInput(1, &input, sizeof(INPUT));
    }

    /**
     * \brief Releases the Windows key.
     */
    void release_winkey() {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_RWIN;
        input.ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(1, &input, sizeof(INPUT));
    }

    /**
     * \brief Presses and releases a combination of keys.
     * \param keys The virtual-key codes to press and release.
     */
     /**
     * \brief Presses and releases a combination of keys.
     * \param keys The virtual-key codes to press and release.
     * \return true if every input event was sent; otherwise false.
     */
    [[nodiscard]] bool send_key_combination(
        std::span<const std::uint8_t> keys
    ) {
        if (keys.empty()) {
            std::cerr
                << "Unable to send key combination: "
                "no key values were provided.\n";

            return false;
        }

        const std::size_t key_count = keys.size();

        if (key_count > UINT_MAX / 2) {
            std::cerr
                << "Unable to send key combination "
                << '[' << format_key_values(keys) << "]: "
                << "too many key values were provided.\n";

            return false;
        }

        const std::size_t total_inputs = key_count * 2;
        const UINT input_count =
            static_cast<UINT>(total_inputs);

        std::vector<INPUT> inputs(total_inputs);

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

        SetLastError(ERROR_SUCCESS);

        const UINT sent = SendInput(
            input_count,
            inputs.data(),
            sizeof(INPUT)
        );

        if (sent != input_count) {
            const DWORD error_code = GetLastError();

            std::cerr
                << "Unable to send key combination ["
                << format_key_values(keys)
                << "]. Sent "
                << sent
                << " of "
                << inputs.size()
                << " input events. Error: "
                << error_code
                << ".\n";

            return false;
        }

        return true;
    }

}