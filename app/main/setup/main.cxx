/**
 * \file main.cxx
 * \brief Creates missing installation shortcuts, then initializes or opens Settings.
 *
 * The installation root is the directory that contains this executable.
 * `config\auto_core.ini` is the only initialization sentinel.
 */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <objbase.h>
#include <shlobj.h>

#include <filesystem>
#include <string>

namespace {

void show_error(const wchar_t* message) {
    MessageBoxW(nullptr, message, L"Auto Core Setup", MB_OK | MB_ICONERROR);
}

std::wstring module_path() {
    std::wstring path(MAX_PATH, L'\0');
    for (;;) {
        const DWORD copied = GetModuleFileNameW(
            nullptr,
            path.data(),
            static_cast<DWORD>(path.size())
        );
        if (copied == 0) {
            return {};
        }
        if (copied < path.size()) {
            path.resize(copied);
            return path;
        }
        path.resize(path.size() * 2);
    }
}

void release(IUnknown* pointer) noexcept {
    if (pointer != nullptr) {
        pointer->Release();
    }
}

bool create_shortcut(
    const std::filesystem::path& link_path,
    const std::filesystem::path& target,
    const std::filesystem::path& working_directory
) {
    IShellLinkW* link = nullptr;
    IPersistFile* file = nullptr;
    bool saved = false;

    const HRESULT created = CoCreateInstance(
        CLSID_ShellLink,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&link)
    );
    if (SUCCEEDED(created)) {
        const HRESULT path_set = link->SetPath(target.c_str());
        const HRESULT directory_set =
            SUCCEEDED(path_set)
                ? link->SetWorkingDirectory(working_directory.c_str())
                : path_set;
        if (SUCCEEDED(directory_set)) {
            const HRESULT queried = link->QueryInterface(IID_PPV_ARGS(&file));
            if (SUCCEEDED(queried)) {
                saved = SUCCEEDED(file->Save(link_path.c_str(), TRUE));
            }
        }
    }

    release(file);
    release(link);
    return saved;
}

bool is_file(const std::filesystem::path& path) {
    std::error_code exists_error;
    return std::filesystem::is_regular_file(path, exists_error);
}

bool launch_settings(
    const std::filesystem::path& executable,
    const std::filesystem::path& working_directory
) {
    STARTUPINFOW startup {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process {};

    if (CreateProcessW(
            executable.c_str(),
            nullptr,
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            working_directory.c_str(),
            &startup,
            &process
        ) == FALSE) {
        return false;
    }

    AllowSetForegroundWindow(process.dwProcessId);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

bool launch_and_wait(
    const std::filesystem::path& executable,
    const std::filesystem::path& working_directory
) {
    STARTUPINFOW startup {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process {};

    if (CreateProcessW(
            executable.c_str(),
            nullptr,
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            working_directory.c_str(),
            &startup,
            &process
        ) == FALSE) {
        return false;
    }

    AllowSetForegroundWindow(process.dwProcessId);
    CloseHandle(process.hThread);
    const DWORD waited = WaitForSingleObject(process.hProcess, INFINITE);
    CloseHandle(process.hProcess);
    return waited == WAIT_OBJECT_0;
}

bool ensure_console() {
    if (GetConsoleWindow() == nullptr && AllocConsole() == FALSE) {
        return false;
    }
    SetConsoleTitleW(L"Auto Core Setup");
    return GetConsoleWindow() != nullptr;
}

void write_console(const wchar_t* text) {
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written = 0;
    WriteConsoleW(
        output,
        text,
        static_cast<DWORD>(std::char_traits<wchar_t>::length(text)),
        &written,
        nullptr
    );
}

void wait_for_key() {
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    FlushConsoleInputBuffer(input);
    for (;;) {
        INPUT_RECORD record {};
        DWORD read = 0;
        if (ReadConsoleInputW(input, &record, 1, &read) == FALSE || read == 0) {
            return;
        }
        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
            return;
        }
    }
}

void show_complete() {
    write_console(
        L"\n"
        L"Setup complete.\n"
        L"\n"
        L"Auto Core and Auto Core Settings shortcuts have been created.\n"
        L"\n"
        L"Pin Auto Core to your taskbar, preferably in positions 1\u201310, "
        L"then launch it to begin using Auto Core.\n"
        L"\n"
        L"Press any key to exit."
    );
    wait_for_key();
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    const std::wstring image = module_path();
    if (image.empty()) {
        show_error(L"Could not resolve the Setup executable path.");
        return 1;
    }

    const std::filesystem::path root =
        std::filesystem::path {image}.parent_path();
    const std::filesystem::path bin = root / L"bin";
    const std::filesystem::path auto_core = bin / L"auto_core.exe";
    const std::filesystem::path settings = bin / L"auto_core_settings.exe";
    const std::filesystem::path initialization = bin / L"auto_core_init.exe";
    const std::filesystem::path sentinel = root / L"config" / L"auto_core.ini";

    if (!is_file(auto_core) || !is_file(settings)) {
        show_error(
            L"Could not find bin\\auto_core.exe and "
            L"bin\\auto_core_settings.exe beside this executable."
        );
        return 1;
    }

    const std::filesystem::path auto_core_link = root / L"Auto Core.lnk";
    const std::filesystem::path settings_link = root / L"Auto Core Settings.lnk";
    const bool need_auto_core_link = !is_file(auto_core_link);
    const bool need_settings_link = !is_file(settings_link);
    if (need_auto_core_link || need_settings_link) {
        const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(com)) {
            show_error(L"Could not initialize COM.");
            return 1;
        }

        const bool shortcuts_saved =
            (!need_auto_core_link ||
             create_shortcut(auto_core_link, auto_core, bin)) &&
            (!need_settings_link ||
             create_shortcut(settings_link, settings, bin));
        CoUninitialize();
        if (!shortcuts_saved) {
            show_error(L"Could not create the shortcuts.");
            return 1;
        }
    }

    if (is_file(sentinel)) {
        if (!launch_settings(settings, bin)) {
            show_error(L"Could not launch Auto Core Settings.");
            return 1;
        }
        return 0;
    }

    if (!is_file(initialization)) {
        show_error(
            L"Could not find bin\\auto_core_init.exe beside this executable."
        );
        return 1;
    }

    if (!ensure_console()) {
        show_error(L"Could not open the Setup console.");
        return 1;
    }

    if (!launch_and_wait(initialization, bin)) {
        show_error(L"Could not launch Auto Core initialization.");
        return 1;
    }
    if (!is_file(sentinel)) {
        show_error(L"Auto Core initialization did not complete.");
        return 1;
    }

    show_complete();
    return 0;
}
