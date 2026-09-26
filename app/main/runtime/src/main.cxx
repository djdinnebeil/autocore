/**
 * \file main.cxx
 * \brief Entry point: configuration, console, crash recovery, and message loop.
 *
 * \author DJ, Lily, Daniel, Jose, Tabby
 */
import std;
import <Windows.h>;

import auto_core.core.config;
import auto_core.core.paths;

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

    [[nodiscard]]
    bool auto_core_ini_exists() {
        std::error_code error;
        return std::filesystem::exists(
            ac::paths::config_directory() / "auto_core.ini",
            error
        );
    }

    [[nodiscard]]
    bool components_list_exists() {
        std::error_code error;
        return std::filesystem::exists(
            ac::paths::components_list_file(),
            error
        );
    }

    [[nodiscard]]
    int run_and_wait(const std::filesystem::path& executable_path) {
        STARTUPINFOW startup_info {};
        startup_info.cb = sizeof(startup_info);
        PROCESS_INFORMATION process_info {};
        std::wstring command_line =
            L"\"" + executable_path.wstring() + L"\"";
        if (!CreateProcessW(
                executable_path.c_str(),
                command_line.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                executable_path.parent_path().c_str(),
                &startup_info,
                &process_info
            )) {
            std::cerr
                << "Unable to start "
                << executable_path.string()
                << '\n';
            return 1;
        }
        CloseHandle(process_info.hThread);
        WaitForSingleObject(process_info.hProcess, INFINITE);
        DWORD exit_code = 1;
        if (!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
            CloseHandle(process_info.hProcess);
            return 1;
        }
        CloseHandle(process_info.hProcess);
        return static_cast<int>(exit_code);
    }

    [[nodiscard]]
    bool ensure_auto_core_initialized() {
        if (auto_core_ini_exists()) {
            return true;
        }
        const auto helper =
            ac::paths::bin_directory() / "auto_core_config.exe";
        std::cout
            << "config/auto_core.ini is missing. Launching auto_core_config.exe.\n";
        if (run_and_wait(helper) != 0 || !auto_core_ini_exists()) {
            std::cerr
                << "Auto Core is not initialized. "
                   "config/auto_core.ini was not created.\n";
            return false;
        }
        return true;
    }

    [[nodiscard]]
    bool ensure_components_initialized() {
        if (components_list_exists()) {
            return true;
        }
        const auto helper =
            ac::paths::bin_directory() / "components_editor.exe";
        std::cout
            << "components.list is missing. Launching "
               "components_editor.exe.\n";
        if (run_and_wait(helper) != 0 || !components_list_exists()) {
            std::cerr
                << "components.list was not created. "
                   "Auto Core cannot start without a component catalog.\n";
            return false;
        }
        return true;
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

    if (!ensure_auto_core_initialized()) {
        return 1;
    }
    if (!ensure_components_initialized()) {
        return 1;
    }

    ac::config::initialize_core_settings();
    if (!ac::config::core_settings_report().empty()) {
        auto_core.log_print(
            "{}",
            ac::config::core_settings_report()
        );
    }

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
 
