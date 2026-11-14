module auto_core.core.console;

import std;
import auto_core.core.encoding;
import auto_core.core.keyboard;
import auto_core.taskbar;
import <Windows.h>;
import <conio.h>;
import <cstdio>;

namespace ac::console {

    namespace {

        bool wait_for_foreground(const HWND expected_window) noexcept {
            constexpr auto activation_timeout =
                std::chrono::milliseconds {500};
            constexpr auto poll_interval =
                std::chrono::milliseconds {10};

            const auto deadline =
                std::chrono::steady_clock::now() + activation_timeout;

            do {
                if (GetForegroundWindow() == expected_window &&
                    !IsIconic(expected_window)) {
                    return true;
                }

                std::this_thread::sleep_for(poll_interval);
            } while (std::chrono::steady_clock::now() < deadline);

            return GetForegroundWindow() == expected_window &&
                !IsIconic(expected_window);
        }

        void give_keyboard_focus(const HWND target_window) noexcept {
            const DWORD current_thread = GetCurrentThreadId();
            const DWORD target_thread =
                GetWindowThreadProcessId(target_window, nullptr);
            const bool attached = target_thread != 0 &&
                target_thread != current_thread &&
                AttachThreadInput(current_thread, target_thread, TRUE);
            (void)SetFocus(target_window);
            if (attached) {
                (void)AttachThreadInput(
                    current_thread,
                    target_thread,
                    FALSE
                );
            }
        }

        bool activate_directly(const HWND target_window) noexcept {
            if (IsIconic(target_window)) {
                (void)ShowWindow(target_window, SW_RESTORE);
            }

            if (GetForegroundWindow() != target_window) {
                (void)BringWindowToTop(target_window);
                (void)SetForegroundWindow(target_window);
                if (!wait_for_foreground(target_window)) {
                    return false;
                }
            }

            give_keyboard_focus(target_window);
            return true;
        }

        bool activate_with_attached_input(
            const HWND target_window
        ) noexcept {
            const HWND foreground_window = GetForegroundWindow();
            if (foreground_window == nullptr) {
                return false;
            }

            const DWORD current_thread = GetCurrentThreadId();
            const DWORD foreground_thread =
                GetWindowThreadProcessId(foreground_window, nullptr);

            if (
                foreground_thread == 0 ||
                foreground_thread == current_thread
            ) {
                return false;
            }

            if (!AttachThreadInput(
                current_thread,
                foreground_thread,
                TRUE
            )) {
                return false;
            }

            if (IsIconic(target_window)) {
                (void)ShowWindow(target_window, SW_RESTORE);
            }
            (void)BringWindowToTop(target_window);
            (void)SetForegroundWindow(target_window);
            give_keyboard_focus(target_window);

            (void)AttachThreadInput(
                current_thread,
                foreground_thread,
                FALSE
            );

            return wait_for_foreground(target_window);
        }

        void notify_activation_required(const HWND window) noexcept {
            FLASHWINFO information {
                .cbSize = sizeof(FLASHWINFO),
                .hwnd = window,
                .dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG,
                .uCount = 3,
                .dwTimeout = 0
            };
            (void)FlashWindowEx(&information);
        }

        bool bind_console_streams() noexcept {
            FILE* stream = nullptr;
            (void)freopen_s(&stream, "CONIN$", "r", stdin);
            (void)freopen_s(&stream, "CONOUT$", "w", stdout);
            (void)freopen_s(&stream, "CONOUT$", "w", stderr);
            std::cin.clear();
            std::cout.clear();
            std::cerr.clear();

            const HWND attached = GetConsoleWindow();
            return attached != nullptr && IsWindow(attached);
        }

        bool attach_parent_console() noexcept {
            const HWND existing = GetConsoleWindow();
            if (existing != nullptr && IsWindow(existing)) {
                return true;
            }

            if (AttachConsole(ATTACH_PARENT_PROCESS) != FALSE) {
                return bind_console_streams();
            }

            if (AllocConsole() == FALSE) {
                return false;
            }

            return bind_console_streams();
        }

        bool is_console_handle(const HANDLE handle) noexcept {
            if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
                return false;
            }

            DWORD mode = 0;
            return GetConsoleMode(handle, &mode) != FALSE;
        }

        bool write_console(
            const HANDLE output,
            const std::wstring_view text
        ) noexcept {
            if (text.empty()) {
                return true;
            }

            DWORD written = 0;
            return WriteConsoleW(
                output,
                text.data(),
                static_cast<DWORD>(text.size()),
                &written,
                nullptr
            ) != FALSE && written == text.size();
        }

        class InputOwnership {
        public:
            InputOwnership(const InputOwnership&) = delete;
            InputOwnership& operator=(const InputOwnership&) = delete;
            InputOwnership(InputOwnership&& other) noexcept
                : mutex_ {std::exchange(other.mutex_, nullptr)} {}
            InputOwnership& operator=(InputOwnership&&) = delete;

            ~InputOwnership() {
                if (mutex_ == nullptr) {
                    return;
                }
                ReleaseMutex(mutex_);
                CloseHandle(mutex_);
            }

            [[nodiscard]] static std::expected<InputOwnership, LineError>
                try_acquire() {
                const HANDLE mutex = CreateMutexW(
                    nullptr,
                    FALSE,
                    L"Local\\AutoCore.ConsoleInput"
                );
                if (mutex == nullptr) {
                    return std::unexpected(LineError::read_failed);
                }

                const DWORD wait_result = WaitForSingleObject(mutex, 0);
                if (
                    wait_result == WAIT_OBJECT_0 ||
                    wait_result == WAIT_ABANDONED
                ) {
                    return InputOwnership {mutex};
                }

                CloseHandle(mutex);
                if (wait_result == WAIT_TIMEOUT) {
                    return std::unexpected(LineError::busy);
                }
                return std::unexpected(LineError::read_failed);
            }

        private:
            explicit InputOwnership(const HANDLE mutex) noexcept
                : mutex_ {mutex} {}

            HANDLE mutex_ = nullptr;
        };

        std::expected<std::string, LineError> read_console_line(
            const HANDLE input,
            const HANDLE output
        ) {
            std::wstring buffer;

            while (true) {
                INPUT_RECORD record {};
                DWORD read = 0;
                if (
                    ReadConsoleInputW(input, &record, 1, &read) == FALSE ||
                    read != 1
                ) {
                    return std::unexpected(LineError::read_failed);
                }

                if (
                    record.EventType != KEY_EVENT ||
                    record.Event.KeyEvent.bKeyDown == FALSE
                ) {
                    continue;
                }

                const KEY_EVENT_RECORD& key = record.Event.KeyEvent;
                const WORD repeat = key.wRepeatCount == 0
                    ? WORD {1}
                    : key.wRepeatCount;

                if (key.wVirtualKeyCode == VK_ESCAPE) {
                    if (!write_console(output, L"\r\n")) {
                        return std::unexpected(LineError::read_failed);
                    }
                    return std::unexpected(LineError::cancelled);
                }

                if (key.wVirtualKeyCode == VK_RETURN) {
                    if (!write_console(output, L"\r\n")) {
                        return std::unexpected(LineError::read_failed);
                    }
                    try {
                        return ac::encoding::to_utf8(buffer);
                    }
                    catch (const std::exception&) {
                        return std::unexpected(LineError::read_failed);
                    }
                }

                if (key.wVirtualKeyCode == VK_BACK) {
                    const std::size_t count = repeat < buffer.size()
                        ? static_cast<std::size_t>(repeat)
                        : buffer.size();
                    buffer.resize(buffer.size() - count);
                    if (
                        count > 0 &&
                        !write_console(
                            output,
                            std::wstring(count, L'\b') +
                                std::wstring(count, L' ') +
                                std::wstring(count, L'\b')
                        )
                    ) {
                        return std::unexpected(LineError::read_failed);
                    }
                    continue;
                }

                if (key.uChar.UnicodeChar < 32) {
                    continue;
                }

                buffer.append(repeat, key.uChar.UnicodeChar);
                if (!write_console(
                    output,
                    std::wstring(repeat, key.uChar.UnicodeChar)
                )) {
                    return std::unexpected(LineError::read_failed);
                }
            }
        }

    }

    std::string_view error_message(const Error error) noexcept {
        switch (error) {
        case Error::console_unavailable:
            return "The Auto Core console is unavailable";
        case Error::activation_failed:
            return "Unable to activate the Auto Core console";
        case Error::window_unavailable:
            return "The previous foreground window is unavailable";
        case Error::window_activation_failed:
            return "Unable to restore the previous foreground window";
        case Error::winkey_activation_failed:
            return "Unable to activate the Auto Core console through Win+number";
        }

        return "Unknown console error";
    }

    WindowHandle window() noexcept {
        return GetConsoleWindow();
    }

    std::expected<void, Error> activate() noexcept {
        if (!attach_parent_console()) {
            return std::unexpected(Error::console_unavailable);
        }

        const HWND console_window = GetConsoleWindow();

        if (console_window == nullptr || !IsWindow(console_window)) {
            return std::unexpected(Error::console_unavailable);
        }

        if (activate_directly(console_window)) {
            return {};
        }

        if (activate_with_attached_input(console_window)) {
            return {};
        }

        notify_activation_required(console_window);
        return std::unexpected(Error::activation_failed);
    }

    std::expected<void, Error>
        activate_window(const WindowHandle target_window) noexcept {
        const HWND target = static_cast<HWND>(target_window);

        if (target == nullptr || !IsWindow(target)) {
            return std::unexpected(Error::window_unavailable);
        }

        if (activate_directly(target)) {
            return {};
        }

        if (activate()) {
            if (activate_directly(target)) {
                return {};
            }
        }

        if (activate_with_attached_input(target)) {
            return {};
        }

        notify_activation_required(target);
        return std::unexpected(Error::window_activation_failed);
    }

    std::expected<WindowHandle, Error> focus_for_prompt() {
        const HWND previous_window = GetForegroundWindow();

        while (_kbhit()) {
            (void)_getch();
        }
        std::cin.clear();

        if (auto result = activate(); !result) {
            return std::unexpected(result.error());
        }

        return previous_window;
    }

    std::expected<WindowHandle, Error>
        focus_for_prompt_via_winkey() {
        const HWND previous_window = GetForegroundWindow();
        if (!attach_parent_console()) {
            return std::unexpected(Error::console_unavailable);
        }

        const HWND console_window = GetConsoleWindow();

        if (console_window == nullptr || !IsWindow(console_window)) {
            return std::unexpected(Error::console_unavailable);
        }

        while (_kbhit()) {
            (void)_getch();
        }
        std::cin.clear();

        if (previous_window == console_window) {
            return previous_window;
        }

        const auto position =
            ac::taskbar::get_native_taskbar_position("auto_core");
        const bool winkey_sent = position &&
            ac::keyboard::send_taskbar_position(position->value);

        if (winkey_sent && wait_for_foreground(console_window)) {
            return previous_window;
        }

        if (auto fallback = activate(); !fallback) {
            return std::unexpected(fallback.error());
        }
        return previous_window;
    }

    std::expected<void, Error>
        restore_focus(const WindowHandle previous_window) noexcept {
        return activate_window(previous_window);
    }

    std::string_view error_message(const LineError error) noexcept {
        switch (error) {
        case LineError::cancelled:
            return "Console input was cancelled";
        case LineError::busy:
            return "Console input is already in use";
        case LineError::unavailable:
            return "Console input is unavailable";
        case LineError::read_failed:
            return "Console input could not be read";
        }

        return "Unknown console input error";
    }

    std::expected<std::string, LineError> read_line() {
        const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
        if (!is_console_handle(input)) {
            return std::unexpected(LineError::unavailable);
        }

        const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        if (!is_console_handle(output)) {
            return std::unexpected(LineError::read_failed);
        }

        auto ownership = InputOwnership::try_acquire();
        if (!ownership) {
            return std::unexpected(ownership.error());
        }

        std::cout.flush();
        std::cerr.flush();
        return read_console_line(input, output);
    }
}
