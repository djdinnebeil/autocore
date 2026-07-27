module journey;
import std;
import config;
import logger;
import print;
import keyboard;
import <Windows.h>;

/**
 * \brief Creates a new process for the specified executable path.
 *
 * \param path The path to the executable.
 * \return True if the process was created successfully, false otherwise.
 */
bool ac::core::create_process(const std::wstring& path) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessW(path.c_str(), NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        ac::print("CreateProcess failed - GetLastError = {}", GetLastError());
        return false;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

/**
 * \brief Starts the journey process.
 *
 * This function starts the journey process by creating a new process for the specified executable.
 * It waits for the child process to terminate before returning.
 */
void start_journey() {
    STARTUPINFOW si_journey;
    PROCESS_INFORMATION pi_journey;

    ZeroMemory(&si_journey, sizeof(si_journey));
    si_journey.cb = sizeof(si_journey);

    ZeroMemory(&pi_journey, sizeof(pi_journey));

    std::wstring current_directory = ac::get_executable_directory();

    std::wstring journey_app_name =
        L"\"" + current_directory + L"\\auto_core.exe\" child";

    if (!CreateProcessW(
        nullptr,
        journey_app_name.data(),
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        current_directory.c_str(),
        &si_journey,
        &pi_journey))
    {
        ac::print("CreateProcess for journey.exe failed ({})", GetLastError());
        return;
    }

    WaitForSingleObject(pi_journey.hProcess, INFINITE);

    CloseHandle(pi_journey.hProcess);
    CloseHandle(pi_journey.hThread);
}
