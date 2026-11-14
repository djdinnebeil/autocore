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
import auto_core.core.shell;

import auto_core.main.application;
import auto_core.main.components;
import auto_core.main.crash_recovery;
import auto_core.main.keyboard_input;
import auto_core.main.keymap.runtime;
import auto_core.main.logging;
import auto_core.core.logging.config;
import auto_core.main.installed_version;
import auto_core.main.program_ready;
import auto_core.main.shutdown_events;
import auto_core.main.version_check;
import auto_core.utility.logging_cleanup;

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

    /**
     * Milliseconds from now until the next local calendar midnight.
     * Falls back to 24 hours when the conversion fails.
     */
    [[nodiscard]]
    DWORD milliseconds_until_next_local_midnight() {
        constexpr DWORD fallback = 24u * 60u * 60u * 1000u;
        SYSTEMTIME local {};
        GetLocalTime(&local);
        local.wHour = 0;
        local.wMinute = 0;
        local.wSecond = 0;
        local.wMilliseconds = 0;

        constexpr int days_in_month[] {
            0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };
        if (local.wMonth < 1 || local.wMonth > 12) {
            return fallback;
        }
        const bool leap =
            (local.wYear % 4 == 0 && local.wYear % 100 != 0) ||
            (local.wYear % 400 == 0);
        int length = days_in_month[local.wMonth];
        if (local.wMonth == 2 && leap) {
            length = 29;
        }
        if (local.wDay < length) {
            ++local.wDay;
        }
        else if (local.wMonth < 12) {
            local.wDay = 1;
            ++local.wMonth;
        }
        else {
            local.wDay = 1;
            local.wMonth = 1;
            ++local.wYear;
        }

        SYSTEMTIME utc {};
        if (!TzSpecificLocalTimeToSystemTime(nullptr, &local, &utc)) {
            return fallback;
        }
        FILETIME next_file_time {};
        if (!SystemTimeToFileTime(&utc, &next_file_time)) {
            return fallback;
        }
        FILETIME now_file_time {};
        GetSystemTimeAsFileTime(&now_file_time);

        ULARGE_INTEGER next {};
        next.LowPart = next_file_time.dwLowDateTime;
        next.HighPart = next_file_time.dwHighDateTime;
        ULARGE_INTEGER now {};
        now.LowPart = now_file_time.dwLowDateTime;
        now.HighPart = now_file_time.dwHighDateTime;
        if (next.QuadPart <= now.QuadPart) {
            return 1000;
        }
        const auto milliseconds =
            (next.QuadPart - now.QuadPart) / 10000ULL;
        if (milliseconds == 0) {
            return 1;
        }
        if (milliseconds > fallback) {
            return fallback;
        }
        return static_cast<DWORD>(milliseconds);
    }

    void log_cleanup_summary(const ac::logging_cleanup::Summary& summary) {
        if (!summary.available) {
            return;
        }
        const int total = summary.retention_deleted + summary.size_deleted;
        if (summary.dry_run) {
            auto_core.log_debug(
                "Dry run. Log cleanup deleted {} file(s) by retention, {} by size, {} total. Projected retention deletions: {}. Projected size deletions: {}.",
                summary.retention_deleted,
                summary.size_deleted,
                total,
                summary.projected_retention,
                summary.projected_size
            );
        }
        else {
            auto_core.log_debug(
                "Log cleanup deleted {} file(s) by retention, {} by size, {} total.",
                summary.retention_deleted,
                summary.size_deleted,
                total
            );
        }
        if (!summary.blocked_by_today) {
            return;
        }
        if (summary.dry_run) {
            auto_core.log_debug(
                "Dry run. Log cleanup deleted {} file(s). Projected retention deletions: {}. Projected size deletions: {}. Remaining size would be {} bytes. Today's protected logs prevented reaching the size limit.",
                total,
                summary.projected_retention,
                summary.projected_size,
                summary.remaining_bytes
            );
            return;
        }
        auto_core.log_debug(
            "Log cleanup deleted {} file(s). Remaining size is {} bytes. Today's protected logs prevented reaching the size limit.",
            total,
            summary.remaining_bytes
        );
    }

    void log_cleanup_result(const ac::logging_cleanup::Result& result) {
        log_cleanup_summary(result.summary);
        switch (result.status) {
        case ac::logging_cleanup::Status::finished:
            return;
        case ac::logging_cleanup::Status::busy:
            auto_core.log_debug("{}", result.detail);
            return;
        case ac::logging_cleanup::Status::failed:
        case ac::logging_cleanup::Status::incomplete:
            auto_core.log_alert("{}", result.detail);
            return;
        }
    }

    [[nodiscard]]
    int run_and_wait(
        const std::filesystem::path& executable_path,
        const std::wstring_view arguments = {}
    ) {
        STARTUPINFOW startup_info {};
        startup_info.cb = sizeof(startup_info);
        PROCESS_INFORMATION process_info {};
        std::wstring command_line =
            L"\"" + executable_path.wstring() + L"\"";
        if (!arguments.empty()) {
            command_line += L' ';
            command_line += arguments;
        }
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
    bool prepare_logging(const bool first_run) {
        ac::logging::config::resolve();

        if (first_run) {
            if (ac::logging::config::logging_ini_missing()) {
                std::cerr << "config/logging_policy.ini was not created.\n";
                return false;
            }
            if (!ac::logging::config::disable_all() &&
                ac::logging::config::component_logging_default() &&
                ac::logging::config::write_logs_to_files()) {
                auto_core.log_debug("Logging initialized");
            }
            auto_core.log_debug("Auto Core initialization started");
            return true;
        }

        if (ac::logging::config::logging_ini_missing()) {
            auto_core.log_alert(
                "config/logging_policy.ini is missing; using built-in logging defaults."
            );
        }
        return true;
    }

    [[nodiscard]]
    bool ensure_auto_core_initialized() {
        if (auto_core_ini_exists()) {
            return true;
        }
        const auto helper =
            ac::paths::bin_directory() / "auto_core_settings.exe";
        if (run_and_wait(helper, L"--initialize") != 0 || !auto_core_ini_exists()) {
            std::cerr
                << "Auto Core is not initialized. "
                   "config/auto_core.ini was not created.\n";
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
    ac::shell::set_process_app_user_model_id();

    if (!acquire_instance_mutex()) {
        std::cerr << "Failed to acquire the Auto Core instance lock\n";
        return 1;
    }

    const bool first_run = !auto_core_ini_exists();
    if (first_run && !ensure_auto_core_initialized()) {
        return 1;
    }
    if (!prepare_logging(first_run)) {
        return 1;
    }
    if (!ac::main::synchronize_installed_version()) {
        auto_core.log_debug(
            "Could not update config/auto_core.ini version."
        );
    }

    ac::config::initialize_core_settings();
    if (!ac::config::core_settings_report().empty()) {
        auto_core.log_alert(
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
    initialize_logging();

    auto component_session =
        ac::main::components::initialize();

    initialize_keymap();
    announce_program_ready();

    std::jthread version_check_thread = start_version_check();
    const HANDLE cleanup_stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::jthread logging_cleanup_thread {[cleanup_stop] {
        try {
            for (;;) {
                try {
                    log_cleanup_result(ac::logging_cleanup::run(
                        {.dry_run = false}
                    ));
                }
                catch (const std::exception& exception) {
                    auto_core.log_alert(
                        "Log cleanup failed: {}",
                        exception.what()
                    );
                }
                catch (...) {
                    auto_core.log_alert("Log cleanup failed.");
                }
                if (cleanup_stop == nullptr) {
                    break;
                }
                const DWORD wait = WaitForSingleObject(
                    cleanup_stop,
                    milliseconds_until_next_local_midnight()
                );
                if (wait != WAIT_TIMEOUT) {
                    break;
                }
            }
        }
        catch (const std::exception& exception) {
            auto_core.log_alert(
                "Log cleanup failed: {}",
                exception.what()
            );
        }
        catch (...) {
            auto_core.log_alert("Log cleanup failed.");
        }
    }};

    run_message_loop();
    if (cleanup_stop != nullptr) {
        SetEvent(cleanup_stop);
    }
    logging_cleanup_thread.join();
    if (cleanup_stop != nullptr) {
        CloseHandle(cleanup_stop);
    }
    if (version_check_thread.joinable()) {
        version_check_thread.join();
    }
    shutdown_logging();

    return 0;
}
 
