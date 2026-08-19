module server;

import std;
import ac_main;
import auto_core.paths;

import <Windows.h>;

PROCESS_INFORMATION pi_server; // Process information for the server process

void start_server() {
    if (pi_server.hProcess != nullptr) {
        auto_core.logg_and_print(
            "Server component is already running"
        );

        return;
    }

    const std::filesystem::path server_path =
        ac::paths::executable_directory() / "server.exe";

    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info {};

    if (!CreateProcessW(
        server_path.c_str(),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        server_path.parent_path().c_str(),
        &startup_info,
        &process_info
    )) {
        const DWORD error = GetLastError();

        auto_core.logg_and_print(
            "Unable to start server component '{}'. Error: {}",
            server_path,
            error
        );

        return;
    }

    CloseHandle(process_info.hThread);
    process_info.hThread = nullptr;

    pi_server = process_info;

    auto_core.logg_and_logg(
        "Server component process started"
    );
}

void stop_server() {
    if (pi_server.hProcess == nullptr) {
        auto_core.logg_and_logg(
            "Server component is not running"
        );

        return;
    }

    if (!TerminateProcess(pi_server.hProcess, 0)) {
        const DWORD error = GetLastError();

        auto_core.logg_and_print(
            "Unable to terminate server component. Error: {}",
            error
        );
    }
    else {
        auto_core.logg_and_logg(
            "Server component terminated during Auto Core shutdown"
        );
    }

    CloseHandle(pi_server.hProcess);
    pi_server = {};
}
