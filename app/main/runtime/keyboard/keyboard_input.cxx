module auto_core.main.keyboard_input;

import auto_core.main.key_codes;
import auto_core.main.keymap;
import auto_core.main.application;
import auto_core.main.taskbar;

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

}

LRESULT CALLBACK keyboard_input::hook_callback(
    const int hook_code,
    const WPARAM message,
    const LPARAM event_data
) {
    if (hook_code < 0 || message != WM_KEYDOWN) {
        return CallNextHookEx(nullptr, hook_code, message, event_data);
    }

    const auto& keyboard_event =
        *reinterpret_cast<const KBDLLHOOKSTRUCT*>(event_data);
    const key_codes::Code key_code =
        normalize_key_code(keyboard_event);

    if (!active_keymap.contains(key_code)) {
        return CallNextHookEx(nullptr, hook_code, message, event_data);
    }

    PostThreadMessage(
        ac::main::main_thread_id,
        key_event_message,
        0,
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

    auto_core.log_main("key_code = {}", key_code);

    if (taskbar.switch_set) {
        taskbar.switch_windows(key_code);
        primary = true;
        return true;
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

    taskbar.switch_keycode = key_code;

    if (!primary && key_code != key_codes::numpad_0) {
        primary = true;
    }

    return true;
}
