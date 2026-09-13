module auto_core.main.logger;

import std;

import auto_core.main.application;
import auto_core.core.clock;
import auto_core.core.logging.config;
import auto_core.core.paths;

import <Windows.h>;

namespace fs = std::filesystem;

namespace {

    std::atomic_bool logger_process_started {false};
    std::atomic_bool logger_shutdown_started {false};
    HANDLE logger_process_handle = nullptr;

    bool start_logger_component() {
        auto_core.logg("Starting logger_ac.exe");

        const std::filesystem::path logger_path =
            ac::paths::executable_directory() / "logger_ac.exe";

        STARTUPINFOW startup_info {};
        startup_info.cb = sizeof(startup_info);

        PROCESS_INFORMATION process_info {};
        std::wstring command_line =
            L"\"" + logger_path.wstring() + L"\"";

        if (!CreateProcessW(
                logger_path.c_str(),
                command_line.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                logger_path.parent_path().c_str(),
                &startup_info,
                &process_info
            )) {
            std::cerr
                << "Unable to start logger_ac.exe. GetLastError = "
                << GetLastError()
                << '\n';
            return false;
        }

        CloseHandle(process_info.hThread);
        logger_process_handle = process_info.hProcess;
        return true;
    }

}

void initialize_logger_component() {
    auto_core.logg(
        "Main session started {}",
        ac::clock::format_datetime(auto_core.session_start())
    );

    fs::create_directories(
        ac::logging::config::directory()
    );

    if (ac::logging::config::enabled()) {
        if (start_logger_component()) {
            logger_process_started.store(true);
            auto_core.connect_to_logger();
        }
    }
    else {
        auto_core.logg("Central logging is disabled");
    }

    if (ac::logging::config::write_to_console()) {
        auto_core.logg_and_logg(
            "***send logg to output enabled***"
        );
    }

    auto_core.loggnl_and_loggnl(
        std::string {ac::logging::config::configuration_report()}
    );
}

void shutdown_logger_component() {
    if (logger_shutdown_started.exchange(true)) {
        return;
    }

    auto_core.logg_and_logg("Auto Core is shutting down");

    if (!logger_process_started.exchange(false)) {
        return;
    }

    const HANDLE process_handle = logger_process_handle;
    logger_process_handle = nullptr;
    if (process_handle == nullptr) {
        return;
    }

    if (!auto_core.request_logger_shutdown()) {
        std::cerr
            << "Failed to send the termination signal to logger_ac.exe\n";
    }

    constexpr DWORD graceful_shutdown_timeout_ms = 6000;
    const DWORD wait_result = WaitForSingleObject(
        process_handle,
        graceful_shutdown_timeout_ms
    );

    if (wait_result != WAIT_OBJECT_0) {
        if (!TerminateProcess(process_handle, 1)) {
            std::cerr
                << "Unable to terminate logger_ac.exe. GetLastError = "
                << GetLastError()
                << '\n';
        }
        else {
            WaitForSingleObject(process_handle, 1000);
        }
    }

    CloseHandle(process_handle);
}
