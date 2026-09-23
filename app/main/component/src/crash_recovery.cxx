module auto_core.main.crash_recovery;

import std;

import auto_core.core.clock;
import auto_core.core.ini;
import auto_core.main.application;
import auto_core.main.logger;
import auto_core.core.paths;

import <Windows.h>;
import <filesystem>;
import <fstream>;
import <iostream>;
import <format>;

namespace fs = std::filesystem;

namespace {

    const fs::path crash_directory =
        ac::paths::installation_root() / "crash";

    const fs::path crash_marker_path =
        crash_directory / ".crash";

    volatile LONG crash_recovery_claimed = 0;

    UINT crash_dialog_default_button() {
        const auto configuration = ac::ini::read(
            ac::paths::config_directory() / "crash_recovery.ini"
        );
        if (!configuration) {
            auto_core.log_and_print(
                "config/crash_recovery.ini is missing or unreadable. Run "
                "crash_recovery_config.exe to generate it. Using default "
                "response no; the file will not be created."
            );
            return MB_DEFBUTTON2;
        }

        const auto response = configuration->find(
            "dialog", "default_response"
        );
        if (!response) {
            auto_core.log_and_print(
                "config/crash_recovery.ini is missing [dialog] "
                "default_response. Run crash_recovery_config.exe to repair "
                "it. Using no; the file will not be rewritten."
            );
            return MB_DEFBUTTON2;
        }
        if (*response == "no") {
            return MB_DEFBUTTON2;
        }
        if (*response == "yes") {
            return MB_DEFBUTTON1;
        }

        auto_core.log_and_print(
            "config/crash_recovery.ini [dialog] default_response must be "
            "'yes' or 'no'. Run crash_recovery_config.exe to repair it. "
            "Using no; the file will not be rewritten."
        );
        return MB_DEFBUTTON2;
    }

}

/**
 * \brief Checks if a crash log exists and prompts the user if it does.
 *
 * This function pauses startup if a crash marker is detected and asks the user
 * whether Auto Core should continue. Yes removes the marker; No leaves it so
 * the next start prompts again.
 *
 * \return true when startup may continue; otherwise false.
 */
bool check_for_previous_crash() {
    if (!fs::exists(crash_marker_path)) {
        return true;
    }

    const std::wstring message = std::format(
        L"Auto Core previously encountered a fatal error.\n\n"
        L"Do you want to continue starting Auto Core?"
    );

    const int response = MessageBoxW(
        nullptr,
        message.c_str(),
        L"Auto Core Crash Recovery",
        MB_YESNO |
        MB_ICONWARNING |
        crash_dialog_default_button() |
        MB_TASKMODAL
    );

    if (response != IDYES) {
        return false;
    }

    std::error_code remove_error;
    fs::remove(crash_marker_path, remove_error);
    if (remove_error) {
        std::cerr
            << "Unable to remove crash/.crash after acknowledgement: "
            << remove_error.message()
            << '\n';
    }

    return true;
}

namespace {

bool write_crash_marker(const std::string_view crash_marker) {
    if (crash_marker.size() > (std::numeric_limits<DWORD>::max)()) {
        return false;
    }

    std::error_code directory_error;
    fs::create_directories(crash_directory, directory_error);
    if (directory_error) {
        return false;
    }

    const HANDLE marker_file = CreateFileW(
        crash_marker_path.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (marker_file == INVALID_HANDLE_VALUE) {
        return false;
    }

    const DWORD marker_size =
        static_cast<DWORD>(crash_marker.size());

    DWORD bytes_written = 0;

    const bool marker_written = WriteFile(
        marker_file,
        crash_marker.data(),
        marker_size,
        &bytes_written,
        nullptr
    ) && bytes_written == marker_size;

    const bool marker_flushed =
        marker_written && FlushFileBuffers(marker_file);

    const bool marker_closed =
        CloseHandle(marker_file);

    return marker_flushed && marker_closed;
}

void restart_program(const std::string& error_report) {
    auto_core.log_and_log(error_report);

    const fs::path executable_path =
        ac::paths::bin_directory() / "auto_core.exe";

    if (!ac::main::create_process(
        executable_path,
        {},
        CREATE_NEW_CONSOLE
    )) {
        auto_core.log_and_log(
            "Unable to restart auto_core.exe"
        );

        return;
    }

    auto_core.log_and_log(
        "Restarting auto_core.exe at {}",
        ac::clock::get_timestamp_with_seconds()
    );

    close_program_noninteractive();
    shutdown_logger_component();
    ExitProcess(1);
}

bool write_crash_log(
    const fs::path& crash_files_directory,
    const std::string_view crash_report
) {
    const fs::path crash_log_path =
        crash_files_directory / "crash.log";

    std::ofstream crash_log(crash_log_path);

    if (!crash_log) {
        return false;
    }

    crash_log << crash_report << '\n';
    crash_log.flush();

    return crash_log.good();
}

/**
 * \brief Generates a unique crash-artifact directory name.
 *
 * Ensures that crash artifacts do not overwrite existing files.
 *
 * \return The generated crash-artifact directory name.
 */
std::string get_crash_artifact_directory_name() {
    std::string crash_name = ac::clock::get_date_iso() + "_crash";
    fs::path crash_path = crash_directory / crash_name;
    int counter = 0;
    while (fs::exists(crash_path)) {
        counter++;
        crash_name = ac::clock::get_date_iso() + "_crash_" + std::to_string(counter);
        crash_path = crash_directory / crash_name;
    }
    return crash_name;
}

/**
 * \brief Unhandled exception handler.
 *
 * Handles unhandled exceptions, preserves diagnostic artifacts, and restarts
 * the program.
 *
 * \param exceptionInfo Pointer to the exception information.
 * \return The exception execution handler.
 */
LONG WINAPI unhandled_exception_handler(
    EXCEPTION_POINTERS* exceptionInfo
) {
    if (InterlockedCompareExchange(
            &crash_recovery_claimed,
            1,
            0
        ) != 0) {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    const std::string artifact_directory_name =
        get_crash_artifact_directory_name();

    const std::string crash_datetime =
        ac::clock::get_datetime();

    const std::string crash_marker = std::format(
        "Program crash on {}",
        crash_datetime
    );

    const std::string error_report = std::format(
        "Unhandled exception occurred."
        "\nProgram crash on {}"
        "\nException Code: {}"
        "\nException Address: {}",
        crash_datetime,
        exceptionInfo->ExceptionRecord->ExceptionCode,
        reinterpret_cast<uintptr_t>(
            exceptionInfo->ExceptionRecord->ExceptionAddress
        )
    );

    if (!write_crash_marker(crash_marker)) {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    std::error_code directory_error;

    const fs::path crash_files_directory =
        crash_directory / artifact_directory_name;

    fs::create_directories(
        crash_files_directory,
        directory_error
    );

    if (directory_error) {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    write_crash_log(
        crash_files_directory,
        error_report
    );

    const fs::path current_exe_path =
        ac::paths::bin_directory() /
        "auto_core.exe";

    const fs::path copy_exe_path =
        crash_files_directory /
        "auto_core.exe";

    const fs::path current_pdb_path =
        ac::paths::installation_root() /
        "symbols" /
        "auto_core.pdb";

    const fs::path copy_pdb_path =
        crash_files_directory /
        "auto_core.pdb";

    CopyFileW(
        current_exe_path.c_str(),
        copy_exe_path.c_str(),
        FALSE
    );

    CopyFileW(
        current_pdb_path.c_str(),
        copy_pdb_path.c_str(),
        FALSE
    );

    restart_program(error_report);

    return EXCEPTION_EXECUTE_HANDLER;
}

}

void enable_automatic_crash_recovery() {
    SetUnhandledExceptionFilter(unhandled_exception_handler);
}
