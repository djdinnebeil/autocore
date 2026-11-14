/**
 * \file component_dll_search.cxx
 * \brief Configures sibling app\ DLL search before component static initialization.
 *
 * Mirrors the benchmark initializer: #pragma init_seg(lib), then
 * SetDefaultDllDirectories and AddDllDirectory. This file does not import
 * Auto Core, call a vendor DLL, or depend on another dynamic initializer.
 * A failure is reported and the process stops.
 */
#include <windows.h>

#include <stdio.h>

#include <delayimp.h>

#pragma comment(lib, "delayimp.lib")
#pragma warning(disable : 4073)
#pragma init_seg(lib)

static wchar_t g_configured_app_directory[32768] = {};

static void write_console_or_file(const wchar_t* message) {
    const HANDLE error = GetStdHandle(STD_ERROR_HANDLE);
    if (error == nullptr || error == INVALID_HANDLE_VALUE || message == nullptr) {
        return;
    }
    DWORD written = 0;
    DWORD mode = 0;
    if (GetConsoleMode(error, &mode)) {
        WriteConsoleW(
            error,
            message,
            static_cast<DWORD>(wcslen(message)),
            &written,
            nullptr
        );
        WriteConsoleW(error, L"\r\n", 2, &written, nullptr);
        return;
    }
    char narrow[4096] = {};
    const int bytes = WideCharToMultiByte(
        CP_UTF8,
        0,
        message,
        -1,
        narrow,
        static_cast<int>(sizeof(narrow) - 2),
        nullptr,
        nullptr
    );
    if (bytes <= 1) {
        return;
    }
    narrow[bytes - 1] = '\r';
    narrow[bytes] = '\n';
    WriteFile(
        error,
        narrow,
        static_cast<DWORD>(bytes + 1),
        &written,
        nullptr
    );
}

[[noreturn]]
static void report_and_stop(const wchar_t* message) {
    write_console_or_file(message);
    const HMODULE user32 = LoadLibraryW(L"user32.dll");
    if (user32 != nullptr) {
        using MessageBoxFunction = int(WINAPI*)(HWND, LPCWSTR, LPCWSTR, UINT);
        const auto message_box = reinterpret_cast<MessageBoxFunction>(
            GetProcAddress(user32, "MessageBoxW")
        );
        if (message_box != nullptr) {
            message_box(
                nullptr,
                message,
                L"Auto Core",
                MB_OK | MB_ICONERROR | MB_SETFOREGROUND
            );
        }
    }
    ExitProcess(1);
}

static void append_windows_error(wchar_t* message, const size_t capacity) {
    const DWORD error = GetLastError();
    wchar_t system_message[512] = {};
    DWORD length = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        0,
        system_message,
        static_cast<DWORD>(sizeof(system_message) / sizeof(system_message[0])),
        nullptr
    );
    while (length > 0 &&
           (system_message[length - 1] == L'\r' ||
            system_message[length - 1] == L'\n')) {
        system_message[length - 1] = L'\0';
        --length;
    }
    wchar_t suffix[640] = {};
    swprintf_s(suffix, L" Windows error %lu: %s", error, system_message);
    wcscat_s(message, capacity, suffix);
}

static bool configure_sibling_app_directory() {
    wchar_t path[32768];
    const DWORD length = GetModuleFileNameW(nullptr, path, 32768);
    if (length == 0 || length >= 32768) {
        return false;
    }
    wchar_t* slash = wcsrchr(path, L'\\');
    if (slash == nullptr) {
        return false;
    }
    *slash = L'\0';
    slash = wcsrchr(path, L'\\');
    if (slash == nullptr) {
        return false;
    }
    *slash = L'\0';
    if (wcscat_s(path, 32768, L"\\app") != 0) {
        return false;
    }
    if (wcscpy_s(g_configured_app_directory, 32768, path) != 0) {
        return false;
    }
    if (!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)) {
        return false;
    }
    return AddDllDirectory(g_configured_app_directory) != nullptr;
}

struct EarlyDllSearch {
    EarlyDllSearch() {
        if (configure_sibling_app_directory()) {
            return;
        }
        wchar_t message[4096] =
            L"Auto Core could not configure shared DLL search.";
        if (g_configured_app_directory[0] != L'\0') {
            wcscat_s(message, L" Expected directory: ");
            wcscat_s(message, g_configured_app_directory);
            wcscat_s(message, L".");
        }
        append_windows_error(message, sizeof(message) / sizeof(message[0]));
        report_and_stop(message);
    }
};

static EarlyDllSearch g_early_dll_search;

extern "C" FARPROC WINAPI auto_core_delay_load_failure(
    unsigned notify,
    PDelayLoadInfo info
) {
    if (info != nullptr &&
        (notify == dliFailLoadLib || notify == dliFailGetProc)) {
        wchar_t dll_name[256] = L"the shared DLL";
        if (info->szDll != nullptr) {
            MultiByteToWideChar(
                CP_ACP,
                0,
                info->szDll,
                -1,
                dll_name,
                static_cast<int>(sizeof(dll_name) / sizeof(dll_name[0]))
            );
        }
        wchar_t message[4096] = {};
        if (notify == dliFailGetProc) {
            wchar_t export_name[256] = L"an export";
            if (info->dlp.fImportByName && info->dlp.szProcName != nullptr) {
                MultiByteToWideChar(
                    CP_ACP,
                    0,
                    info->dlp.szProcName,
                    -1,
                    export_name,
                    static_cast<int>(sizeof(export_name) / sizeof(export_name[0]))
                );
            }
            swprintf_s(
                message,
                L"Auto Core loaded %s but the required export %s is missing.",
                dll_name,
                export_name
            );
        }
        else {
            swprintf_s(message, L"Auto Core could not load %s.", dll_name);
            if (g_configured_app_directory[0] != L'\0') {
                wcscat_s(message, L" Searched ");
                wcscat_s(message, g_configured_app_directory);
                wcscat_s(message, L".");
            }
            append_windows_error(message, sizeof(message) / sizeof(message[0]));
        }
        report_and_stop(message);
    }
    return nullptr;
}

extern "C" const PfnDliHook __pfnDliFailureHook2 = auto_core_delay_load_failure;
