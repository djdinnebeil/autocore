/**
 * \file main.cxx
 * \brief Refreshes installation-root shortcuts and launches Settings.
 *
 * The installation root is the directory that contains this executable.
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

    std::error_code exists_error;
    if (!std::filesystem::is_regular_file(auto_core, exists_error) ||
        !std::filesystem::is_regular_file(settings, exists_error)) {
        show_error(
            L"Could not find bin\\auto_core.exe and "
            L"bin\\auto_core_settings.exe beside this executable."
        );
        return 1;
    }

    const HRESULT initialized =
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized)) {
        show_error(L"Could not initialize COM.");
        return 1;
    }

    const bool shortcuts_saved =
        create_shortcut(root / L"Auto Core.lnk", auto_core, bin) &&
        create_shortcut(root / L"Auto Core Settings.lnk", settings, bin);
    if (!shortcuts_saved) {
        CoUninitialize();
        show_error(L"Could not create the shortcuts.");
        return 1;
    }

    const bool launched = launch_settings(settings, bin);
    CoUninitialize();
    if (!launched) {
        show_error(L"Could not launch Auto Core Settings.");
        return 1;
    }
    return 0;
}
