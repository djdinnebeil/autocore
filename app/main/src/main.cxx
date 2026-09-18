/**
 * \file main.cxx
 * \brief Entry point: configuration, console, crash recovery, and message loop.
 *
 * \author DJ, Lily, Daniel, Jose, Tabby
 */
import std;
import <Windows.h>;

import auto_core.core.config;

import auto_core.main.application;
import auto_core.main.components;
import auto_core.main.crash_recovery;
import auto_core.main.keyboard_input;
import auto_core.main.keymap.runtime;
import auto_core.main.logger;
import auto_core.main.program_ready;
import auto_core.main.shutdown_events;

namespace {
    constexpr wchar_t instance_mutex_name[] = L"Local\\AutoCore.main";

    /**
     * Waits for a previous auto_core.exe to exit. The handle is not released
     * so the mutex stays owned through static destructors until process exit.
     */
    [[nodiscard]]
    bool acquire_instance_mutex() {
        const HANDLE mutex = CreateMutexW(
            nullptr,
            FALSE,
            instance_mutex_name
        );
        if (mutex == nullptr) {
            return false;
        }

        const DWORD wait = WaitForSingleObject(mutex, INFINITE);
        if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED) {
            CloseHandle(mutex);
            return false;
        }

        return true;
    }

    void initialize_process_environment() {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleTitleW(L"Auto Core");
    }

    void initialize_application_runtime() {
        ac::main::main_thread_id = GetCurrentThreadId();

        ac::main::keyboard_hook = SetWindowsHookEx(
            WH_KEYBOARD_LL,
            keyboard_input::hook_callback,
            nullptr,
            0
        );

        if (!initialize_shutdown_events()) {
            std::cerr << "Failed to initialize shutdown event handling\n";
        }
    }

    void run_message_loop() {
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0) > 0) {
            try {
                if (process_shutdown_event(msg)) {
                    continue;
                }
                if (keyboard_input::process_key_event(msg)) {
                    continue;
                }
            }
            catch (const std::exception& e) {
                auto_core.print("caught exception: {}", e.what());
            }
            catch (...) {
                auto_core.print("uncaught exception");
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

/**
 * \brief Process entry: configuration, crash recovery, hook, children, keymap, message loop.
 *
 * \return `1` if the user declines crash continue; otherwise `0`.
 */
int main() {

    if (!acquire_instance_mutex()) {
        std::cerr << "Failed to acquire the Auto Core instance lock\n";
        return 1;
    }

    ac::config::initialize_core_settings();

    initialize_process_environment();
    if (!check_for_previous_crash()) {
        return 1;
    }
    enable_automatic_crash_recovery();

    initialize_application_runtime();
    initialize_logger_component();

    auto component_session =
        ac::main::components::initialize();

    initialize_keymap();
    announce_program_ready();
    run_message_loop();
    shutdown_logger_component();

    return 0;
}
 
