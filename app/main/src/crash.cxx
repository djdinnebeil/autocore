module crash;

import std;

import clock;
import config;
import ac_component;
import print;
import ac_main;
import session;

import <Windows.h>;
import <filesystem>;
import <fstream>;
import <iostream>;
import <format>;

std::string crash_log = R"(.\crash\crash.log)";
std::string crash_folder = R"(.\crash\)";

/**
 * \brief Checks if a crash log exists and prompts the user if it does.
 *
 * This function pauses the operation of the program if a crash log is detected.
 * It ensures that Auto Core does not get caught in an infinite loop by prompting
 * the user to acknowledge the crash before continuing. This behavior is crucial
 * for preventing the program from repeatedly restarting without intervention.
 */
void crash_check() {
    if (std::filesystem::exists(crash_log)) {
        auto_core.print("Crash detected --- press any key to continue");
        std::cin.get(); // Pause and wait for user input
    }
}

/**
 * \brief Restarts the program after a crash.
 *
 * Logs the crash information, copies necessary files, and restarts the program.
 *
 * \param error_report The error report to log.
 */
void restart_program(const std::string& error_report) {
    std::ofstream log_crash(
        crash_log,
        std::ios::app
    );

    if (log_crash.is_open()) {
        const std::string crash_report = std::format(
            "{}\nProgram crash on {}",
            error_report,
            ac::session::make_datetime()
        );

        auto_core.logg_and_logg(crash_report);
        log_crash << crash_report << '\n';
    }

    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info {};

    const std::wstring_view executable_directory =
        ac::config::executable_directory();

    std::wstring restart_command = std::format(
        L"\"{}\\auto_core.exe\"",
        executable_directory
    );

    if (!CreateProcessW(
        nullptr,
        restart_command.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_CONSOLE,
        nullptr,
        nullptr,
        &startup_info,
        &process_info
    )) {
        auto_core.logg_and_logg(
            "Unable to restart auto_core.exe. Error: {}",
            GetLastError()
        );

        return;
    }

    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    auto_core.logg_and_logg(
        "Restarting auto_core.exe at {}",
        ac::clock::get_timestamp_with_seconds()
    );

    close_program();
    ExitProcess(1);
}

/**
 * \brief Generates a unique crash dump file name.
 *
 * Ensures that the crash dump file name is unique to avoid overwriting existing files.
 *
 * \return The generated crash dump file name.
 */
std::string get_crash_name() {
    std::string crash_name = ac::clock::get_date_iso() + "_crash";
    std::string crash_path = crash_folder + crash_name + ".dmp";
    int counter = 0;
    while (std::filesystem::exists(crash_path)) {
        counter++;
        crash_name = ac::clock::get_date_iso() + "_crash_" + std::to_string(counter);
        crash_path = crash_folder + crash_name + ".dmp";
    }
    return crash_name;
}

/**
 * \brief Unhandled exception handler.
 *
 * Handles unhandled exceptions, generates a crash dump, and restarts the program.
 *
 * \param exceptionInfo Pointer to the exception information.
 * \return The exception execution handler.
 */
LONG WINAPI unhandled_exception_handler(EXCEPTION_POINTERS* exceptionInfo) {
    // Create the crash folder if it does not exist
    std::wstring crash_folder_wstr = std::wstring(crash_folder.begin(), crash_folder.end());
    CreateDirectoryW(crash_folder_wstr.c_str(), NULL);
    // Generate a unique crash dump file name
    std::string crash_dmt_name = get_crash_name();
    std::wstring crash_log_name = std::wstring(crash_dmt_name.begin(), crash_dmt_name.end());
    std::wstring filepath = crash_folder_wstr + crash_log_name + L".dmp";
    std::wstring crash_folder_crash_files = crash_folder_wstr + crash_log_name + L"\\";
    CreateDirectoryW(crash_folder_crash_files.c_str(), NULL);
    // Copy the executable and PDB files
    std::wstring current_exe_path = LR"(.\auto_core.exe)";
    std::wstring copy_exe_path = crash_folder_crash_files + L"auto_core.exe";
    std::wstring current_pdb_path = LR"(.\symbols\auto_core.pdb)";
    std::wstring copy_pdb_path = crash_folder_crash_files + L"auto_core.pdb";
    CopyFileW(current_exe_path.c_str(), copy_exe_path.c_str(), FALSE);
    CopyFileW(current_pdb_path.c_str(), copy_pdb_path.c_str(), FALSE);
    // Log the error report and restart the program
    std::string error_report = "Unhandled exception occurred.";
    error_report += std::format("\nSee {} for a crash log", crash_dmt_name);
    error_report += "\nException Code: " + std::to_string(exceptionInfo->ExceptionRecord->ExceptionCode);
    error_report += "\nException Address: " + std::to_string(reinterpret_cast<uintptr_t>(exceptionInfo->ExceptionRecord->ExceptionAddress));
    restart_program(error_report);
    return EXCEPTION_EXECUTE_HANDLER;
}
