module auto_core.main.logger;

import std;

import auto_core.main.application;
import auto_core.main.components;
import auto_core.core.clock;
import auto_core.core.logging.config;
import auto_core.core.paths;

import <Windows.h>;

namespace fs = std::filesystem;

namespace {

    std::atomic_bool logger_process_started {false};
    std::atomic_bool logger_shutdown_started {false};
    HANDLE logger_process_handle = nullptr;
    HANDLE logger_job_handle = nullptr;

    void close_handle(HANDLE& handle) noexcept {
        if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
        handle = nullptr;
    }

    bool start_logger_component() {
        auto_core.log("Starting logger_ac.exe");

        const std::filesystem::path logger_path =
            ac::paths::executable_directory() / "logger_ac.exe";

        STARTUPINFOW startup_info {};
        startup_info.cb = sizeof(startup_info);

        PROCESS_INFORMATION process_info {};
        std::wstring command_line =
            L"\"" + logger_path.wstring() + L"\"";

        HANDLE job = CreateJobObjectW(nullptr, nullptr);
        if (job == nullptr) {
            std::cerr
                << "Unable to create the logger job object. GetLastError = "
                << GetLastError()
                << '\n';
            return false;
        }

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_limits {};
        job_limits.BasicLimitInformation.LimitFlags =
            JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(
                job,
                JobObjectExtendedLimitInformation,
                &job_limits,
                sizeof(job_limits)
            )) {
            std::cerr
                << "Unable to configure the logger job object. GetLastError = "
                << GetLastError()
                << '\n';
            CloseHandle(job);
            return false;
        }

        if (!CreateProcessW(
                logger_path.c_str(),
                command_line.data(),
                nullptr,
                nullptr,
                FALSE,
                CREATE_NO_WINDOW | CREATE_SUSPENDED,
                nullptr,
                logger_path.parent_path().c_str(),
                &startup_info,
                &process_info
            )) {
            std::cerr
                << "Unable to start logger_ac.exe. GetLastError = "
                << GetLastError()
                << '\n';
            CloseHandle(job);
            return false;
        }

        if (!AssignProcessToJobObject(job, process_info.hProcess)) {
            const DWORD error = GetLastError();
            (void)TerminateProcess(process_info.hProcess, 1);
            (void)WaitForSingleObject(process_info.hProcess, 1000);
            CloseHandle(process_info.hThread);
            CloseHandle(process_info.hProcess);
            CloseHandle(job);
            std::cerr
                << "Unable to assign logger_ac.exe to its job object. "
                   "GetLastError = "
                << error
                << '\n';
            return false;
        }

        if (ResumeThread(process_info.hThread) == static_cast<DWORD>(-1)) {
            const DWORD error = GetLastError();
            (void)TerminateProcess(process_info.hProcess, 1);
            (void)WaitForSingleObject(process_info.hProcess, 1000);
            CloseHandle(process_info.hThread);
            CloseHandle(process_info.hProcess);
            CloseHandle(job);
            std::cerr
                << "Unable to resume logger_ac.exe. GetLastError = "
                << error
                << '\n';
            return false;
        }

        CloseHandle(process_info.hThread);
        logger_process_handle = process_info.hProcess;
        logger_job_handle = job;
        return true;
    }

}

void initialize_logger_component() {
    auto_core.log(
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
        auto_core.log("Central logging is disabled");
    }

    if (ac::logging::config::write_to_console()) {
        auto_core.log_and_log(
            "***send log to output enabled***"
        );
    }

    auto_core.lognl_and_lognl(
        std::string {ac::logging::config::configuration_report()}
    );
}

void shutdown_logger_component() {
    if (logger_shutdown_started.exchange(true)) {
        return;
    }

    auto_core.log_and_log("Auto Core is shutting down");

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

    const auto configured_timeout =
        ac::main::components::shutdown_timeout();
    const DWORD timeout = static_cast<DWORD>(configured_timeout.count());
    const DWORD wait_result = WaitForSingleObject(process_handle, timeout);

    if (wait_result == WAIT_TIMEOUT) {
        auto_core.log(
            "logger_ac.exe exceeded the configured shutdown timeout; "
            "forcing termination"
        );
        if (!TerminateProcess(process_handle, 1)) {
            std::cerr
                << "Unable to terminate logger_ac.exe. GetLastError = "
                << GetLastError()
                << '\n';
        }
        else {
            (void)WaitForSingleObject(process_handle, 1000);
        }
    }
    else if (wait_result == WAIT_FAILED) {
        std::cerr
            << "Unable to wait for logger_ac.exe. GetLastError = "
            << GetLastError()
            << '\n';
    }

    CloseHandle(process_handle);
    close_handle(logger_job_handle);
}
