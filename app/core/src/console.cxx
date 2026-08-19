module auto_core.console;

import std;
import <Windows.h>;
import <conio.h>;

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
                if (GetForegroundWindow() == expected_window) {
                    return true;
                }

                std::this_thread::sleep_for(poll_interval);
            } while (std::chrono::steady_clock::now() < deadline);

            return GetForegroundWindow() == expected_window;
        }

        bool activate_directly(const HWND target_window) noexcept {
            if (GetForegroundWindow() == target_window) {
                return true;
            }

            if (IsIconic(target_window)) {
                (void)ShowWindow(target_window, SW_RESTORE);
            }

            (void)BringWindowToTop(target_window);
            (void)SetForegroundWindow(target_window);
            return wait_for_foreground(target_window);
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
        }

        return "Unknown console error";
    }

    WindowHandle window() noexcept {
        return GetConsoleWindow();
    }

    std::expected<void, Error> activate() noexcept {
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

    std::expected<void, Error>
        restore_focus(const WindowHandle previous_window) noexcept {
        return activate_window(previous_window);
    }
}
