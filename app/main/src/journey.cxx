module journey;

import std;

import config;
import ac_component;
import print;

import <Windows.h>;

/**
 * \brief Creates a new process for the specified executable path.
 *
 * \param path The path to the executable.
 * \return True if the process was created successfully, false otherwise.
 */
bool ac::main::create_process(const std::wstring& path) {
    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info {};

    if (!CreateProcessW(
        path.c_str(),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &startup_info,
        &process_info)) {
        const DWORD error = GetLastError();

        auto_core.print(
            "CreateProcess failed - GetLastError = {}",
            error
        );

        return false;
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    return true;
}

