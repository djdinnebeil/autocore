/**
 * \file wake_capture.cxx
 * \brief Captures `powercfg /lastwake` before Wake history files are written.
 */
#include "wake_capture.hpp"

#define NOMINMAX
#include <Windows.h>

#include <string>

namespace wake {
namespace {

class Handle {
public:
    Handle() = default;
    explicit Handle(const HANDLE value) noexcept : value {value} {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&& other) noexcept : value {other.value} {
        other.value = nullptr;
    }
    Handle& operator=(Handle&& other) noexcept {
        if (this != &other) {
            reset();
            value = other.value;
            other.value = nullptr;
        }
        return *this;
    }
    ~Handle() {
        reset();
    }

    [[nodiscard]] HANDLE get() const noexcept {
        return value;
    }

    [[nodiscard]] bool valid() const noexcept {
        return value != nullptr && value != INVALID_HANDLE_VALUE;
    }

    void reset() noexcept {
        if (valid()) {
            CloseHandle(value);
        }
        value = nullptr;
    }

private:
    HANDLE value {nullptr};
};

} // namespace

std::optional<std::string> capture_powercfg() {
    SECURITY_ATTRIBUTES inheritable {};
    inheritable.nLength = sizeof(inheritable);
    inheritable.bInheritHandle = TRUE;

    HANDLE read_handle = nullptr;
    HANDLE write_handle = nullptr;
    if (!CreatePipe(&read_handle, &write_handle, &inheritable, 0)) {
        return std::nullopt;
    }
    Handle read {read_handle};
    Handle write {write_handle};
    if (!SetHandleInformation(read.get(), HANDLE_FLAG_INHERIT, 0)) {
        return std::nullopt;
    }

    Handle nul {CreateFileW(
        L"NUL",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        &inheritable,
        OPEN_EXISTING,
        0,
        nullptr
    )};
    if (!nul.valid()) {
        return std::nullopt;
    }

    STARTUPINFOW startup {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = nul.get();
    startup.hStdError = nul.get();
    startup.hStdOutput = write.get();

    wchar_t command[] = L"powercfg.exe /lastwake";
    PROCESS_INFORMATION process {};
    if (!CreateProcessW(
            nullptr,
            command,
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startup,
            &process
        )) {
        return std::nullopt;
    }
    Handle process_handle {process.hProcess};
    Handle thread_handle {process.hThread};
    write.reset();
    nul.reset();

    std::string output;
    char buffer[4096];
    DWORD read_bytes = 0;
    while (ReadFile(read.get(), buffer, sizeof(buffer), &read_bytes, nullptr) &&
           read_bytes != 0) {
        output.append(buffer, read_bytes);
    }
    read.reset();

    if (WaitForSingleObject(process_handle.get(), INFINITE) != WAIT_OBJECT_0) {
        return std::nullopt;
    }
    DWORD exit_code = 1;
    if (!GetExitCodeProcess(process_handle.get(), &exit_code) || exit_code != 0) {
        return std::nullopt;
    }
    if (output.empty()) {
        return std::nullopt;
    }
    return output;
}

} // namespace wake
