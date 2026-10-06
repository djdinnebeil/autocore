module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#include <winver.h>
#include "crash_diagnostics_detail.hpp"

module auto_core.core.crash_diagnostics;

import std;
import auto_core.core.ini;
import auto_core.core.paths;

namespace ac::crash {
namespace {

    constexpr std::size_t path_capacity = 4096;
    constexpr std::size_t text_capacity = 16384;
    constexpr unsigned maximum_collision_attempts = 32;

    using MiniDumpWriteDumpFunction = BOOL (WINAPI*)(
        HANDLE,
        DWORD,
        HANDLE,
        MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION,
        PMINIDUMP_USER_STREAM_INFORMATION,
        PMINIDUMP_CALLBACK_INFORMATION
    );

    struct CachedState {
        std::once_flag initialization;
        bool initialized {false};
        bool diagnostics_enabled {detail::diagnostics_default};
        ULONGLONG start_tick {0};
        HMODULE dbghelp {nullptr};
        MiniDumpWriteDumpFunction write_dump {nullptr};
        DWORD dbghelp_error {ERROR_SUCCESS};
        volatile LONG report_claimed {0};
        std::atomic<Operation> operation {Operation::unspecified};
        std::atomic<State> context_state {State::unspecified};
        wchar_t crash_directory[path_capacity] {};
        wchar_t executable_name[MAX_PATH] {};
        wchar_t executable_stem[MAX_PATH] {};
        char executable_name_utf8[MAX_PATH * 3] {};
        char component_utf8[MAX_PATH * 3] {};
        char product_version[64] {"unknown"};
        char build_id[64] {"unknown"};
        char dll_version[64] {"unknown"};
        char report[text_capacity] {};
    };

    CachedState cache;

    void copy_wide(
        wchar_t* destination,
        const std::size_t capacity,
        const std::wstring_view source
    ) noexcept {
        if (capacity == 0) {
            return;
        }
        const auto count = (std::min)(capacity - 1, source.size());
        std::wmemcpy(destination, source.data(), count);
        destination[count] = L'\0';
    }

    void wide_to_utf8(
        const wchar_t* source,
        char* destination,
        const int capacity
    ) noexcept {
        if (capacity <= 0) {
            return;
        }
        destination[0] = '\0';
        if (source == nullptr || source[0] == L'\0') {
            return;
        }
        if (WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                source,
                -1,
                destination,
                capacity,
                nullptr,
                nullptr
            ) == 0) {
            destination[0] = '\0';
        }
    }

    std::string file_version(const std::filesystem::path& path) {
        DWORD ignored = 0;
        const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &ignored);
        if (size == 0) {
            return "unknown";
        }
        std::vector<std::byte> bytes(size);
        if (!GetFileVersionInfoW(path.c_str(), 0, size, bytes.data())) {
            return "unknown";
        }
        VS_FIXEDFILEINFO* info = nullptr;
        UINT info_size = 0;
        if (!VerQueryValueW(
                bytes.data(),
                L"\\",
                reinterpret_cast<void**>(&info),
                &info_size
            ) || info == nullptr || info_size < sizeof(VS_FIXEDFILEINFO)) {
            return "unknown";
        }
        return std::format(
            "{}.{}.{}.{}",
            HIWORD(info->dwFileVersionMS),
            LOWORD(info->dwFileVersionMS),
            HIWORD(info->dwFileVersionLS),
            LOWORD(info->dwFileVersionLS)
        );
    }

    std::string pe_build_id(const HMODULE module) {
        if (module == nullptr) {
            return "unknown";
        }
        const auto base = reinterpret_cast<const std::byte*>(module);
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
            return "unknown";
        }
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
            base + dos->e_lfanew
        );
        if (nt->Signature != IMAGE_NT_SIGNATURE) {
            return "unknown";
        }
        return std::format(
            "{:08X}-{:08X}",
            nt->FileHeader.TimeDateStamp,
            nt->OptionalHeader.SizeOfImage
        );
    }

    template<std::size_t Size>
    void copy_narrow(char (&destination)[Size], const std::string_view value) {
        const auto count = (std::min)(Size - 1, value.size());
        std::memcpy(destination, value.data(), count);
        destination[count] = '\0';
    }

    const char* operation_name(const Operation value) noexcept {
        switch (value) {
        case Operation::startup: return "startup";
        case Operation::idle: return "idle";
        case Operation::shutdown: return "shutdown";
        default: return "unspecified";
        }
    }

    const char* state_name(const State value) noexcept {
        switch (value) {
        case State::entering: return "entering";
        case State::active: return "active";
        case State::leaving: return "leaving";
        default: return "unspecified";
        }
    }

    struct TextBuilder {
        char* data;
        std::size_t capacity;
        std::size_t used {0};

        void append(const char* format, ...) noexcept {
            if (used >= capacity) {
                return;
            }
            va_list arguments;
            va_start(arguments, format);
            const int written = _vsnprintf_s(
                data + used,
                capacity - used,
                _TRUNCATE,
                format,
                arguments
            );
            va_end(arguments);
            if (written > 0) {
                used += static_cast<std::size_t>(written);
            }
            else {
                used = std::strlen(data);
            }
        }
    };

    void utc_timestamp(
        char* narrow,
        const std::size_t narrow_capacity,
        wchar_t* wide,
        const std::size_t wide_capacity
    ) noexcept {
        SYSTEMTIME time {};
        GetSystemTime(&time);
        _snprintf_s(
            narrow,
            narrow_capacity,
            _TRUNCATE,
            "%04hu%02hu%02huT%02hu%02hu%02hu.%03huZ",
            time.wYear, time.wMonth, time.wDay,
            time.wHour, time.wMinute, time.wSecond, time.wMilliseconds
        );
        _snwprintf_s(
            wide,
            wide_capacity,
            _TRUNCATE,
            L"%04hu%02hu%02huT%02hu%02hu%02hu.%03huZ",
            time.wYear, time.wMonth, time.wDay,
            time.wHour, time.wMinute, time.wSecond, time.wMilliseconds
        );
    }

    bool create_event_directory(
        const wchar_t* timestamp,
        const DWORD process_id,
        wchar_t* destination,
        const std::size_t destination_capacity
    ) noexcept {
        if (CreateDirectoryW(cache.crash_directory, nullptr) == FALSE &&
            GetLastError() != ERROR_ALREADY_EXISTS) {
            return false;
        }

        for (unsigned collision = 0;
             collision < maximum_collision_attempts;
             ++collision) {
            if (collision == 0) {
                _snwprintf_s(
                    destination,
                    destination_capacity,
                    _TRUNCATE,
                    L"%ls\\%ls_%ls_%lu",
                    cache.crash_directory,
                    timestamp,
                    cache.executable_stem,
                    process_id
                );
            }
            else {
                _snwprintf_s(
                    destination,
                    destination_capacity,
                    _TRUNCATE,
                    L"%ls\\%ls_%ls_%lu_%u",
                    cache.crash_directory,
                    timestamp,
                    cache.executable_stem,
                    process_id,
                    collision
                );
            }
            if (CreateDirectoryW(destination, nullptr)) {
                return true;
            }
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                return false;
            }
        }
        return false;
    }

    bool write_text_file(
        const wchar_t* path,
        const char* bytes,
        const DWORD length
    ) noexcept {
        const HANDLE file = CreateFileW(
            path,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (file == INVALID_HANDLE_VALUE) {
            return false;
        }
        DWORD written = 0;
        const bool ok = WriteFile(file, bytes, length, &written, nullptr) &&
            written == length;
        if (ok) {
            (void)FlushFileBuffers(file);
        }
        CloseHandle(file);
        return ok;
    }

    std::size_t build_report(
        EXCEPTION_POINTERS* exception,
        const char* timestamp,
        const char* dump_status,
        const DWORD dump_error
    ) noexcept {
        cache.report[0] = '\0';
        TextBuilder output {cache.report, text_capacity};

        DWORD code = 0;
        DWORD flags = 0;
        std::uintptr_t address = 0;
        if (exception != nullptr && exception->ExceptionRecord != nullptr) {
            code = exception->ExceptionRecord->ExceptionCode;
            flags = exception->ExceptionRecord->ExceptionFlags;
            address = reinterpret_cast<std::uintptr_t>(
                exception->ExceptionRecord->ExceptionAddress
            );
        }

        MEMORY_BASIC_INFORMATION memory {};
        std::uintptr_t module_base = 0;
        wchar_t module_path[path_capacity] {};
        char module_name[MAX_PATH * 3] {"unknown"};
        if (address != 0 && VirtualQuery(
                reinterpret_cast<const void*>(address),
                &memory,
                sizeof(memory)
            ) == sizeof(memory)) {
            module_base = reinterpret_cast<std::uintptr_t>(
                memory.AllocationBase
            );
            if (GetModuleFileNameW(
                    static_cast<HMODULE>(memory.AllocationBase),
                    module_path,
                    static_cast<DWORD>(std::size(module_path))
                ) != 0) {
                const wchar_t* leaf = std::wcsrchr(module_path, L'\\');
                wide_to_utf8(
                    leaf == nullptr ? module_path : leaf + 1,
                    module_name,
                    static_cast<int>(std::size(module_name))
                );
            }
        }

        output.append(
            "[crash]\r\n"
            "format_version = 1\r\n"
            "timestamp_utc = %s\r\n"
            "executable = %s\r\n"
            "process_id = %lu\r\n"
            "thread_id = %lu\r\n"
            "uptime_ms = %llu\r\n\r\n",
            timestamp,
            cache.executable_name_utf8,
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            GetTickCount64() - cache.start_tick
        );
        output.append(
            "[build]\r\n"
            "product_version = %s\r\n"
            "build_id = %s\r\n"
            "architecture = %s\r\n"
            "auto_core_dll_version = %s\r\n\r\n",
            cache.product_version,
            cache.build_id,
#if defined(_M_X64)
            "x64",
#elif defined(_M_IX86)
            "x86",
#else
            "unknown",
#endif
            cache.dll_version
        );
        output.append(
            "[exception]\r\n"
            "exception_code = 0x%08lX\r\n"
            "exception_flags = 0x%08lX\r\n"
            "fault_address = 0x%llX\r\n"
            "fault_module = %s\r\n"
            "fault_module_base = 0x%llX\r\n"
            "fault_module_offset = 0x%llX\r\n\r\n",
            code,
            flags,
            static_cast<unsigned long long>(address),
            module_name,
            static_cast<unsigned long long>(module_base),
            static_cast<unsigned long long>(
                module_base == 0 ? 0 : address - module_base
            )
        );
#if defined(_M_X64)
        if (exception != nullptr && exception->ContextRecord != nullptr) {
            output.append(
                "[registers]\r\n"
                "rip = 0x%llX\r\n"
                "rsp = 0x%llX\r\n"
                "rbp = 0x%llX\r\n\r\n",
                exception->ContextRecord->Rip,
                exception->ContextRecord->Rsp,
                exception->ContextRecord->Rbp
            );
        }
#endif
        output.append(
            "[minidump]\r\n"
            "status = %s\r\n"
            "win32_error_if_failed = %lu\r\n\r\n"
            "[context]\r\n"
            "component = %s\r\n"
            "operation = %s\r\n"
            "state = %s\r\n",
            dump_status,
            dump_error,
            cache.component_utf8,
            operation_name(cache.operation.load(std::memory_order_relaxed)),
            state_name(cache.context_state.load(std::memory_order_relaxed))
        );
        return output.used;
    }

    LONG WINAPI shared_exception_filter(EXCEPTION_POINTERS* exception) {
        write_report(exception);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void initialize_once() {
        cache.start_tick = GetTickCount64();

        const auto configuration = ac::ini::read(
            ac::paths::config_directory() / "crash_recovery.ini"
        );
        std::optional<std::string_view> configured;
        if (configuration) {
            configured = configuration->find(
                "crash_recovery", "crash_diagnostics"
            );
        }
        cache.diagnostics_enabled = detail::resolve_enabled(configured);
        cache.initialized = true;
        if (!cache.diagnostics_enabled) {
            return;
        }

        const auto crash_path = ac::paths::installation_root() / "crash";
        copy_wide(
            cache.crash_directory,
            std::size(cache.crash_directory),
            crash_path.native()
        );

        wchar_t image_path[path_capacity] {};
        GetModuleFileNameW(
            nullptr,
            image_path,
            static_cast<DWORD>(std::size(image_path))
        );
        const std::filesystem::path executable_path {image_path};
        copy_wide(
            cache.executable_name,
            std::size(cache.executable_name),
            executable_path.filename().native()
        );
        copy_wide(
            cache.executable_stem,
            std::size(cache.executable_stem),
            executable_path.stem().native()
        );
        wide_to_utf8(
            cache.executable_name,
            cache.executable_name_utf8,
            static_cast<int>(std::size(cache.executable_name_utf8))
        );

        std::wstring component {cache.executable_stem};
        if (component.ends_with(L"_ac")) {
            component.resize(component.size() - 3);
        }
        char component_text[MAX_PATH * 3] {};
        wide_to_utf8(
            component.c_str(),
            component_text,
            static_cast<int>(std::size(component_text))
        );
        copy_narrow(cache.component_utf8, component_text);

        copy_narrow(cache.product_version, file_version(executable_path));
        copy_narrow(cache.build_id, pe_build_id(GetModuleHandleW(nullptr)));

        HMODULE core_module = nullptr;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&cache),
            &core_module
        );
        wchar_t core_path[path_capacity] {};
        if (core_module != nullptr && GetModuleFileNameW(
                core_module,
                core_path,
                static_cast<DWORD>(std::size(core_path))
            ) != 0) {
            copy_narrow(
                cache.dll_version,
                file_version(std::filesystem::path {core_path})
            );
        }

        const auto dbghelp_path =
            ac::paths::bin_directory() / "dbghelp.dll";
        cache.dbghelp = LoadLibraryExW(
            dbghelp_path.c_str(),
            nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
        );
        if (cache.dbghelp == nullptr) {
            cache.dbghelp_error = GetLastError();
        }
        else {
            cache.write_dump = reinterpret_cast<MiniDumpWriteDumpFunction>(
                GetProcAddress(cache.dbghelp, "MiniDumpWriteDump")
            );
            if (cache.write_dump == nullptr) {
                cache.dbghelp_error = ERROR_PROC_NOT_FOUND;
            }
        }

        SetUnhandledExceptionFilter(shared_exception_filter);
    }

} // namespace

void initialize() noexcept {
    try {
        std::call_once(cache.initialization, initialize_once);
    }
    catch (...) {
        cache.initialized = true;
        cache.diagnostics_enabled = detail::diagnostics_default;
    }
}

void set_context(const Operation operation, const State state) noexcept {
    cache.operation.store(operation, std::memory_order_relaxed);
    cache.context_state.store(state, std::memory_order_relaxed);
}

bool enabled() noexcept {
    initialize();
    return cache.diagnostics_enabled;
}

void write_report(EXCEPTION_POINTERS* exception) noexcept {
    if (!cache.initialized) {
        initialize();
    }
    if (!cache.diagnostics_enabled || cache.crash_directory[0] == L'\0') {
        return;
    }
    if (InterlockedCompareExchange(&cache.report_claimed, 1, 0) != 0) {
        return;
    }

    char timestamp[40] {};
    wchar_t timestamp_wide[40] {};
    utc_timestamp(
        timestamp,
        std::size(timestamp),
        timestamp_wide,
        std::size(timestamp_wide)
    );

    wchar_t event_directory[path_capacity] {};
    if (!create_event_directory(
            timestamp_wide,
            GetCurrentProcessId(),
            event_directory,
            std::size(event_directory)
        )) {
        return;
    }

    wchar_t text_path[path_capacity] {};
    wchar_t dump_path[path_capacity] {};
    _snwprintf_s(
        text_path,
        std::size(text_path),
        _TRUNCATE,
        L"%ls\\crash.txt",
        event_directory
    );
    _snwprintf_s(
        dump_path,
        std::size(dump_path),
        _TRUNCATE,
        L"%ls\\crash.dmp",
        event_directory
    );

    auto report_size = build_report(exception, timestamp, "pending", 0);
    (void)write_text_file(
        text_path,
        cache.report,
        static_cast<DWORD>(report_size)
    );

    DWORD dump_error = cache.dbghelp_error;
    bool dump_written = false;
    if (cache.write_dump != nullptr) {
        const HANDLE dump = CreateFileW(
            dump_path,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (dump == INVALID_HANDLE_VALUE) {
            dump_error = GetLastError();
        }
        else {
            MINIDUMP_EXCEPTION_INFORMATION exception_information {
                GetCurrentThreadId(),
                exception,
                FALSE
            };
            constexpr auto dump_type = static_cast<MINIDUMP_TYPE>(
                MiniDumpNormal |
                MiniDumpWithThreadInfo |
                MiniDumpWithUnloadedModules |
                MiniDumpWithoutOptionalData
            );
            dump_written = cache.write_dump(
                GetCurrentProcess(),
                GetCurrentProcessId(),
                dump,
                dump_type,
                exception == nullptr ? nullptr : &exception_information,
                nullptr,
                nullptr
            ) != FALSE;
            dump_error = dump_written ? ERROR_SUCCESS : GetLastError();
            CloseHandle(dump);
            if (!dump_written) {
                DeleteFileW(dump_path);
            }
        }
    }

    report_size = build_report(
        exception,
        timestamp,
        dump_written ? "written" : "failed",
        dump_error
    );
    (void)write_text_file(
        text_path,
        cache.report,
        static_cast<DWORD>(report_size)
    );
}

} // namespace ac::crash
