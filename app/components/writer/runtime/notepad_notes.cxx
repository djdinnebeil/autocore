module writer_commands;

import std;
import auto_core.core.paths;
import writer_component;

import <Windows.h>;

namespace {

std::wstring quote_argument(std::wstring_view value) {
    std::wstring quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back(L'"');
    quoted.append(value);
    quoted.push_back(L'"');
    return quoted;
}

int run_writer_editor(std::string_view argument) {
    const std::filesystem::path executable =
        ac::paths::bin_directory() / "writer_editor.exe";
    const std::wstring wide_argument {argument.begin(), argument.end()};
    std::wstring command =
        quote_argument(executable.wstring()) + L" " +
        quote_argument(wide_argument);

    STARTUPINFOW startup {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process {};
    if (!CreateProcessW(
            executable.c_str(),
            command.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            executable.parent_path().c_str(),
            &startup,
            &process
        )) {
        writer_component().log_print("Unable to start writer_editor.exe.");
        return 1;
    }

    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!GetExitCodeProcess(process.hProcess, &exit_code)) {
        CloseHandle(process.hProcess);
        writer_component().log_print(
            "Unable to read the exit code from writer_editor.exe."
        );
        return 1;
    }
    CloseHandle(process.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        writer_component().log_print(
            "writer_editor.exe --daily-note exited {}.",
            code
        );
    }
    return code;
}

} // namespace

void writer_actions::create_or_open_daily_note_in_notepad() {
    (void)run_writer_editor("--daily-note");
}
