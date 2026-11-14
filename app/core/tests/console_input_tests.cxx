#include "catch_amalgamated.hpp"

#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <thread>
#include <Windows.h>

import auto_core.core.console;

namespace {

constexpr wchar_t child_environment[] = L"AC_CONSOLE_INPUT_CHILD";
constexpr wchar_t input_mutex_name[] = L"Local\\AutoCore.ConsoleInput";

bool in_isolated_child() {
    wchar_t value[8] {};
    const DWORD length = GetEnvironmentVariableW(
        child_environment,
        value,
        static_cast<DWORD>(std::size(value))
    );
    return length > 0 && std::wstring_view {value, length} == L"1";
}

std::wstring quote_argument(const std::wstring_view text) {
    std::wstring quoted;
    quoted.push_back(L'"');
    for (const wchar_t character : text) {
        if (character == L'"') {
            quoted += L"\\\"";
        }
        else {
            quoted.push_back(character);
        }
    }
    quoted.push_back(L'"');
    return quoted;
}

void write_record(const HANDLE input, const INPUT_RECORD& record) {
    DWORD written = 0;
    REQUIRE(WriteConsoleInputW(input, &record, 1, &written) != FALSE);
    REQUIRE(written == 1);
}

void write_key(
    const HANDLE input,
    const WORD virtual_key,
    const wchar_t character,
    const WORD repeat = 1,
    const bool key_down = true
) {
    INPUT_RECORD record {};
    record.EventType = KEY_EVENT;
    record.Event.KeyEvent.bKeyDown = key_down ? TRUE : FALSE;
    record.Event.KeyEvent.wRepeatCount = repeat;
    record.Event.KeyEvent.wVirtualKeyCode = virtual_key;
    record.Event.KeyEvent.wVirtualScanCode = static_cast<WORD>(
        MapVirtualKeyW(virtual_key, MAPVK_VK_TO_VSC)
    );
    record.Event.KeyEvent.uChar.UnicodeChar = character;
    write_record(input, record);
}

void write_ignored_events(const HANDLE input) {
    write_key(input, L'9', L'9', 1, false);

    INPUT_RECORD mouse {};
    mouse.EventType = MOUSE_EVENT;
    mouse.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
    write_record(input, mouse);

    INPUT_RECORD focus {};
    focus.EventType = FOCUS_EVENT;
    focus.Event.FocusEvent.bSetFocus = TRUE;
    write_record(input, focus);

    write_key(input, VK_LEFT, 0);
    write_key(input, L'A', L'\x01');
}

class StandardHandleGuard {
public:
    StandardHandleGuard(const DWORD standard_handle, const HANDLE replacement)
        : standard_handle_ {standard_handle}
        , original_ {GetStdHandle(standard_handle)} {
        REQUIRE(SetStdHandle(standard_handle, replacement) != FALSE);
    }

    ~StandardHandleGuard() {
        SetStdHandle(standard_handle_, original_);
    }

    StandardHandleGuard(const StandardHandleGuard&) = delete;
    StandardHandleGuard& operator=(const StandardHandleGuard&) = delete;

private:
    DWORD standard_handle_;
    HANDLE original_;
};

class OtherThreadMutex {
public:
    OtherThreadMutex()
        : ready_ {CreateEventW(nullptr, TRUE, FALSE, nullptr)}
        , release_ {CreateEventW(nullptr, TRUE, FALSE, nullptr)} {
        REQUIRE(ready_ != nullptr);
        REQUIRE(release_ != nullptr);
        thread_ = std::thread([this] { hold(); });
    }

    ~OtherThreadMutex() {
        if (release_ != nullptr) {
            SetEvent(release_);
        }
        if (thread_.joinable()) {
            thread_.join();
        }
        if (ready_ != nullptr) {
            CloseHandle(ready_);
        }
        if (release_ != nullptr) {
            CloseHandle(release_);
        }
    }

    OtherThreadMutex(const OtherThreadMutex&) = delete;
    OtherThreadMutex& operator=(const OtherThreadMutex&) = delete;

    [[nodiscard]] bool acquired() {
        return WaitForSingleObject(ready_, 5000) == WAIT_OBJECT_0 &&
            failure.empty();
    }

    std::string failure;

private:
    void hold() {
        const HANDLE mutex = CreateMutexW(nullptr, FALSE, input_mutex_name);
        if (mutex == nullptr) {
            failure = "CreateMutexW failed in the owner thread";
            SetEvent(ready_);
            return;
        }

        const DWORD wait_result = WaitForSingleObject(mutex, 5000);
        if (
            wait_result != WAIT_OBJECT_0 &&
            wait_result != WAIT_ABANDONED
        ) {
            failure = "The owner thread did not acquire console input";
            CloseHandle(mutex);
            SetEvent(ready_);
            return;
        }

        SetEvent(ready_);
        WaitForSingleObject(release_, 5000);
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }

    HANDLE ready_ = nullptr;
    HANDLE release_ = nullptr;
    std::thread thread_;
};

COORD cursor(const HANDLE output) {
    CONSOLE_SCREEN_BUFFER_INFO information {};
    REQUIRE(GetConsoleScreenBufferInfo(output, &information) != FALSE);
    return information.dwCursorPosition;
}

void check_echo(
    const HANDLE output,
    const COORD origin,
    const std::wstring_view expected
) {
    std::wstring visible(expected.size(), L'\0');
    DWORD read = 0;
    REQUIRE(ReadConsoleOutputCharacterW(
        output,
        visible.data(),
        static_cast<DWORD>(visible.size()),
        origin,
        &read
    ) != FALSE);
    visible.resize(read);
    CHECK(visible == expected);
}

std::expected<std::string, ac::console::LineError> submit_line(
    const HANDLE output,
    const std::wstring_view echoed
) {
    const COORD origin = cursor(output);
    const auto line = ac::console::read_line();
    if (line && !echoed.empty()) {
        check_echo(output, origin, echoed);
    }
    return line;
}

void run_isolated_checks() {
    const HWND console_window = GetConsoleWindow();
    REQUIRE(console_window != nullptr);
    ShowWindow(console_window, SW_HIDE);

    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    REQUIRE(input != nullptr);
    REQUIRE(input != INVALID_HANDLE_VALUE);
    REQUIRE(output != nullptr);
    REQUIRE(output != INVALID_HANDLE_VALUE);

    DWORD input_mode = 0;
    DWORD output_mode = 0;
    REQUIRE(GetConsoleMode(input, &input_mode) != FALSE);
    REQUIRE(GetConsoleMode(output, &output_mode) != FALSE);
    REQUIRE(FlushConsoleInputBuffer(input) != FALSE);

    {
        HANDLE read_end = nullptr;
        HANDLE write_end = nullptr;
        REQUIRE(CreatePipe(&read_end, &write_end, nullptr, 0) != FALSE);
        {
            const StandardHandleGuard guard {STD_INPUT_HANDLE, read_end};
            const auto line = ac::console::read_line();
            REQUIRE_FALSE(line);
            CHECK(line.error() == ac::console::LineError::unavailable);
        }
        {
            write_key(input, L'7', L'7');
            write_key(input, VK_RETURN, L'\r');
            const StandardHandleGuard guard {STD_OUTPUT_HANDLE, write_end};
            const auto line = ac::console::read_line();
            REQUIRE_FALSE(line);
            CHECK(line.error() == ac::console::LineError::read_failed);
        }
        const auto line = ac::console::read_line();
        REQUIRE(line);
        CHECK(*line == "7");
        CloseHandle(read_end);
        CloseHandle(write_end);
    }

    write_ignored_events(input);
    write_key(input, L'1', L'1');
    write_key(input, L'2', L'2');
    write_key(input, VK_RETURN, L'\r');
    const auto digits = submit_line(output, L"12");
    REQUIRE(digits);
    CHECK(*digits == "12");

    write_key(input, L'2', L'2');
    write_key(input, L'A', L'a');
    write_key(input, VK_RETURN, L'\r');
    const auto invalid = ac::console::read_line();
    REQUIRE(invalid);
    CHECK(*invalid == "2a");

    write_key(input, VK_RETURN, L'\r');
    const auto empty = ac::console::read_line();
    REQUIRE(empty);
    CHECK(empty->empty());

    write_key(input, VK_BACK, L'\b');
    write_key(input, L'1', L'1');
    write_key(input, VK_BACK, L'\b');
    write_key(input, VK_RETURN, L'\r');
    const auto erased = ac::console::read_line();
    REQUIRE(erased);
    CHECK(erased->empty());

    write_key(input, L'1', L'1');
    write_key(input, L'2', L'2');
    write_key(input, L'3', L'3');
    write_key(input, VK_BACK, L'\b', 2);
    write_key(input, VK_RETURN, L'\r');
    const COORD backspace_origin = cursor(output);
    const auto backed = ac::console::read_line();
    REQUIRE(backed);
    CHECK(*backed == "1");
    check_echo(output, backspace_origin, L"1");

    write_key(input, L'8', L'8', 3);
    write_key(input, VK_RETURN, L'\r');
    const auto repeated = submit_line(output, L"888");
    REQUIRE(repeated);
    CHECK(*repeated == "888");

    write_key(input, L'6', L'6');
    write_key(input, VK_ESCAPE, L'\x1b');
    const COORD escape_origin = cursor(output);
    const auto cancelled = ac::console::read_line();
    REQUIRE_FALSE(cancelled);
    CHECK(cancelled.error() == ac::console::LineError::cancelled);
    check_echo(output, escape_origin, L"6");

    {
        OtherThreadMutex owner;
        REQUIRE(owner.acquired());
        write_key(input, L'4', L'4');
        write_key(input, VK_RETURN, L'\r');
        const auto busy = ac::console::read_line();
        REQUIRE_FALSE(busy);
        CHECK(busy.error() == ac::console::LineError::busy);
    }
    const auto after_busy = ac::console::read_line();
    REQUIRE(after_busy);
    CHECK(*after_busy == "4");

    write_key(input, L'5', L'5');
    write_key(input, VK_RETURN, L'\r');
    const auto second = ac::console::read_line();
    REQUIRE(second);
    CHECK(*second == "5");

    DWORD input_after = 0;
    DWORD output_after = 0;
    REQUIRE(GetConsoleMode(input, &input_after) != FALSE);
    REQUIRE(GetConsoleMode(output, &output_after) != FALSE);
    CHECK(input_mode == input_after);
    CHECK(output_mode == output_after);
}

std::wstring executable_path() {
    std::wstring path(32768, L'\0');
    const DWORD length = GetModuleFileNameW(
        nullptr,
        path.data(),
        static_cast<DWORD>(path.size())
    );
    REQUIRE(length > 0);
    REQUIRE(length < path.size());
    path.resize(length);
    return path;
}

class ChildEnvironment {
public:
    ChildEnvironment() {
        REQUIRE(SetEnvironmentVariableW(child_environment, L"1") != FALSE);
    }

    ~ChildEnvironment() {
        SetEnvironmentVariableW(child_environment, nullptr);
    }

    ChildEnvironment(const ChildEnvironment&) = delete;
    ChildEnvironment& operator=(const ChildEnvironment&) = delete;
};

int run_isolated_child(const std::wstring& result_path) {
    const ChildEnvironment environment;

    std::wstring command = quote_argument(executable_path());
    command += L" [console-input-isolated] --reporter compact";
    command += L" --colour-mode none --out ";
    command += quote_argument(result_path);

    STARTUPINFOW startup {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process {};
    const BOOL created = CreateProcessW(
        nullptr,
        command.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_CONSOLE,
        nullptr,
        nullptr,
        &startup,
        &process
    );
    if (created == FALSE) {
        FAIL("CreateProcessW failed with error " << GetLastError());
    }

    const DWORD wait_result = WaitForSingleObject(process.hProcess, 30000);
    if (wait_result != WAIT_OBJECT_0) {
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        FAIL("The isolated console test did not finish");
        return 1;
    }

    DWORD exit_code = 1;
    REQUIRE(GetExitCodeProcess(process.hProcess, &exit_code) != FALSE);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return static_cast<int>(exit_code);
}

std::string file_text(const std::wstring& path) {
    std::ifstream input {std::filesystem::path {path}};
    if (!input) {
        return {};
    }
    return {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
}

} // namespace

TEST_CASE(
    "Console line errors have stable descriptions",
    "[console][input]"
) {
    using ac::console::LineError;

    CHECK(ac::console::error_message(LineError::cancelled) ==
        "Console input was cancelled");
    CHECK(ac::console::error_message(LineError::busy) ==
        "Console input is already in use");
    CHECK(ac::console::error_message(LineError::unavailable) ==
        "Console input is unavailable");
    CHECK(ac::console::error_message(LineError::read_failed) ==
        "Console input could not be read");
}

TEST_CASE(
    "read_line reads an isolated console",
    "[console][input][console-input-isolated]"
) {
    if (in_isolated_child()) {
        run_isolated_checks();
        return;
    }

    wchar_t directory[MAX_PATH] {};
    REQUIRE(GetTempPathW(MAX_PATH, directory) != 0);
    const std::wstring result_path =
        std::wstring {directory} + L"ac_console_input_test.txt";
    DeleteFileW(result_path.c_str());

    const int exit_code = run_isolated_child(result_path);
    if (exit_code != 0) {
        FAIL(file_text(result_path));
    }
    DeleteFileW(result_path.c_str());
}
