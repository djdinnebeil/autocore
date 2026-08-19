module slash;

import std;
import ac_main;
import auto_core.paths;

import <Windows.h>;

void call_slash_exe(
    const std::filesystem::path& executable_path
) {
    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info {};

    if (!CreateProcessW(
        executable_path.c_str(),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        executable_path.parent_path().c_str(),
        &startup_info,
        &process_info
    )) {
        const DWORD error = GetLastError();

        auto_core.print(
            "Unable to start '{}'. Error: {}",
            executable_path,
            error
        );

        return;
    }

    const DWORD wait_result = WaitForSingleObject(
        process_info.hProcess,
        INFINITE
    );

    if (wait_result == WAIT_FAILED) {
        const DWORD error = GetLastError();

        auto_core.print(
            "Unable to wait for '{}'. Error: {}",
            executable_path,
            error
        );
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
}

/**
 * \brief Prints and deletes the contents of the Recycle Bin.
 * \keymap_command
 */
void retrieve_and_delete_recycle_bin() {
    auto_core.logg_and_logg(
        "retrieve_and_delete_recycle_bin()"
    );

    const std::filesystem::path slash_path =
        ac::paths::executable_directory() / "slash.exe";

    call_slash_exe(slash_path);
}

void slash::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("retrieve_and_delete_recycle_bin", &::retrieve_and_delete_recycle_bin);
}
