module crash;

import std;

import auto_core.clock;
import ac_main;
import auto_core.paths;

import <Windows.h>;
import <filesystem>;
import <fstream>;
import <iostream>;
import <format>;

namespace fs = std::filesystem;

namespace {

    const fs::path crash_directory =
        ac::paths::executable_directory() / "crash";

    const fs::path crash_log_path =
        crash_directory / "crash.log";

}

/**
 * \brief Checks if a crash log exists and prompts the user if it does.
 *
 * This function pauses the operation of the program if a crash log is detected.
 * It ensures that Auto Core does not get caught in an infinite loop by prompting
 * the user to acknowledge the crash before continuing. This behavior is crucial
 * for preventing the program from repeatedly restarting without intervention.
 */
void crash_check() {
    if (fs::exists(crash_log_path)) {
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
    std::error_code directory_error;
    fs::create_directories(
        crash_directory,
        directory_error
    );

    std::ofstream log_crash(
        crash_log_path,
        std::ios::app
    );

    if (log_crash.is_open()) {
        const std::string crash_report = std::format(
            "{}\nProgram crash on {}",
            error_report,
            ac::clock::get_datetime()
        );

        auto_core.logg_and_logg(crash_report);
        log_crash << crash_report << '\n';
    }

    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info {};

    const fs::path executable_path =
        ac::paths::executable_directory() / "auto_core.exe";

    std::wstring restart_command =
        L"\"" + executable_path.native() + L"\"";

    if (!CreateProcessW(
        executable_path.c_str(),
        restart_command.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_CONSOLE,
        nullptr,
        ac::paths::executable_directory().c_str(),
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
    fs::path crash_path = crash_directory / (crash_name + ".dmp");
    int counter = 0;
    while (fs::exists(crash_path)) {
        counter++;
        crash_name = ac::clock::get_date_iso() + "_crash_" + std::to_string(counter);
        crash_path = crash_directory / (crash_name + ".dmp");
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
    std::error_code directory_error;
    fs::create_directories(
        crash_directory,
        directory_error
    );

    // Generate a unique crash dump file name
    const std::string crash_dmt_name = get_crash_name();
    const fs::path crash_files_directory =
        crash_directory / crash_dmt_name;

    fs::create_directories(
        crash_files_directory,
        directory_error
    );

    // Copy the executable and PDB files
    const fs::path current_exe_path =
        ac::paths::executable_directory() / "auto_core.exe";
    const fs::path copy_exe_path =
        crash_files_directory / "auto_core.exe";
    const fs::path current_pdb_path =
        ac::paths::executable_directory() /
        "symbols" /
        "auto_core.pdb";
    const fs::path copy_pdb_path =
        crash_files_directory / "auto_core.pdb";

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
