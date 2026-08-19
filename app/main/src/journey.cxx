module journey;

import std;

import ac_main;

import <Windows.h>;

/**
 * \brief Creates a new process for the specified executable path.
 *
 * \param path The path to the executable.
 * \return True if the process was created successfully, false otherwise.
 */
bool ac::main::create_process(
    const std::filesystem::path& executable_path,
    const std::wstring_view arguments
) {
    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info {};

    const std::filesystem::path working_directory =
        executable_path.parent_path();

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
        working_directory.c_str(),
        &startup_info,
        &process_info
    )) {
        const DWORD error = GetLastError();

        auto_core.print(
            "Unable to start '{}'. GetLastError = {}",
            executable_path,
            error
        );

        return false;
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    return true;
}
