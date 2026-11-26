module auto_core.main.keyboard_input;

import auto_core.main.key_codes;
import auto_core.main.keymap;
import auto_core.main.application;
import auto_core.taskbar;
import auto_core.core.keyboard;
import std;

import <Windows.h>;

namespace {

key_codes::Code normalize_key_code(
    const KBDLLHOOKSTRUCT& keyboard_event
) {
    if (keyboard_event.vkCode == key_codes::enter &&
        (keyboard_event.flags & LLKHF_EXTENDED)) {
        return key_codes::numpad_enter;
    }

    return static_cast<key_codes::Code>(keyboard_event.vkCode);
}

bool is_cycle_selection_key(const DWORD vk) noexcept {
    return vk == static_cast<DWORD>('0') || vk == VK_NUMPAD0;
}

bool is_media_or_launch_key(const DWORD vk) noexcept {
    return vk == VK_MEDIA_PLAY_PAUSE ||
        vk == VK_BROWSER_HOME ||
        vk == VK_LAUNCH_MAIL ||
        vk == VK_LAUNCH_APP2;
}

struct SelectionPress {
    bool consuming {false};
    DWORD vk {0};
};

SelectionPress& cycle_selection_press() {
    static SelectionPress press;
    return press;
}

}

LRESULT CALLBACK keyboard_input::hook_callback(
    const int hook_code,
    const WPARAM message,
    const LPARAM event_data
) {
    if (hook_code < 0) {
        return CallNextHookEx(nullptr, hook_code, message, event_data);
    }

    const auto& keyboard_event =
        *reinterpret_cast<const KBDLLHOOKSTRUCT*>(event_data);
    if ((keyboard_event.flags & LLKHF_INJECTED) != 0 &&
        !is_media_or_launch_key(keyboard_event.vkCode)) {
        return CallNextHookEx(nullptr, hook_code, message, event_data);
    }

    const bool key_down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const bool key_up = message == WM_KEYUP || message == WM_SYSKEYUP;
    SelectionPress& selection = cycle_selection_press();
    if (selection.consuming && keyboard_event.vkCode == selection.vk) {
        if (key_up) {
            selection.consuming = false;
        }
        return 1;
    }

    if (!key_down) {
        return CallNextHookEx(nullptr, hook_code, message, event_data);
    }

    const key_codes::Code key_code =
        normalize_key_code(keyboard_event);
    if (ac::taskbar::cycle_uses_native_winkey() &&
        is_cycle_selection_key(keyboard_event.vkCode)) {
        selection.consuming = true;
        selection.vk = keyboard_event.vkCode;
        PostThreadMessage(
            ac::main::main_thread_id,
            key_event_message,
            select_cycle,
            0
        );
        return 1;
    }

    const bool is_bound = active_keymap.contains(key_code);
    if (!is_bound && !ac::taskbar::cycle_session_active()) {
        return CallNextHookEx(nullptr, hook_code, message, event_data);
    }

    PostThreadMessage(
        ac::main::main_thread_id,
        key_event_message,
        is_bound ? dispatch_binding : release_and_replay,
        static_cast<LPARAM>(key_code)
    );
    return 1;
}

bool keyboard_input::process_key_event(const MSG& message) {
    if (message.message != key_event_message) {
        return false;
    }

    const key_codes::Code key_code =
        static_cast<key_codes::Code>(message.lParam);

    if (message.wParam == select_cycle) {
        ac::taskbar::select_cycle();
        return true;
    }

    if (message.wParam == release_and_replay) {
        ac::taskbar::cancel_cycle();
        const std::array<std::uint8_t, 1> key {
            static_cast<std::uint8_t>(key_code)
        };
        (void)ac::keyboard::send_key_combination(key);
        return true;
    }

    auto_core.log_info("key_code = {}", key_code);
    ac::taskbar::note_cycle_shortcut(static_cast<std::uint32_t>(key_code));

    if (ac::taskbar::cycle_session_active() &&
        ac::taskbar::cycle_initiating_key() ==
            static_cast<std::uint32_t>(key_code)) {
        (void)ac::taskbar::continue_cycle();
        return true;
    }

    if (ac::taskbar::cycle_session_active()) {
        ac::taskbar::cancel_cycle();
    }

    if (const auto binding = active_keymap.find(key_code);
        binding != active_keymap.end()) {
        if (primary) {
            binding->second.primary();
        }
        else {
            binding->second.secondary();
        }
    }

    if (!primary && key_code != key_codes::numpad_0) {
        primary = true;
    }

    return true;
}
