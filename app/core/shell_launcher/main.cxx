/**
 * \file main.cxx
 * \brief Breakaway shell launcher.
 *
 * A hosted component starts this process with CREATE_BREAKAWAY_FROM_JOB.
 * ShellExecuteEx then runs outside that component job, preserving the verb,
 * file association, and browser selection of the original call.
 */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <shellapi.h>

#include <string>
#include <string_view>

namespace {

std::wstring argument_value(const int argc, wchar_t** argv, const wchar_t* name) {
    for (int index = 1; index + 1 < argc; ++index) {
        if (std::wstring_view {argv[index]} == name) {
            return argv[index + 1];
        }
    }
    return {};
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr) {
        return static_cast<int>(GetLastError());
    }
    const std::wstring verb = argument_value(argc, argv, L"--verb");
    const std::wstring file = argument_value(argc, argv, L"--file");
    const std::wstring parameters = argument_value(argc, argv, L"--parameters");
    const std::wstring directory = argument_value(argc, argv, L"--directory");
    LocalFree(argv);
    if (file.empty()) {
        return 2;
    }

    SHELLEXECUTEINFOW execution {};
    execution.cbSize = sizeof(execution);
    execution.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    execution.lpVerb = verb.empty() ? nullptr : verb.c_str();
    execution.lpFile = file.c_str();
    execution.lpParameters = parameters.empty() ? nullptr : parameters.c_str();
    execution.lpDirectory = directory.empty() ? nullptr : directory.c_str();
    execution.nShow = SW_SHOWNORMAL;
    if (ShellExecuteExW(&execution) == FALSE ||
        reinterpret_cast<std::intptr_t>(execution.hInstApp) <= 32) {
        const auto code = execution.hInstApp == nullptr
            ? GetLastError()
            : static_cast<DWORD>(reinterpret_cast<std::intptr_t>(execution.hInstApp));
        return code == 0 ? 1 : static_cast<int>(code);
    }
    if (execution.hProcess != nullptr) {
        (void)WaitForInputIdle(execution.hProcess, 1000);
        CloseHandle(execution.hProcess);
    }
    return 0;
}
