module writer_commands;

import std;
import auto_core.core.clock;
import auto_core.core.console;
import auto_core.core.paths;
import auto_core.taskbar;
import writer_component;

import <Windows.h>;

namespace {

namespace fs = std::filesystem;

std::optional<fs::path> daily_note_path() {
    const fs::path notes_directory = ac::paths::notes_directory();
    std::error_code error;
    fs::create_directories(notes_directory, error);

    if (error) {
        writer_component().log_print(
            "Unable to create notes directory '{}': {}",
            notes_directory.string(),
            error.message()
        );
        return std::nullopt;
    }
    return notes_directory / (ac::clock::get_date_iso() + ".txt");
}

bool ensure_file_exists(const fs::path& path) {
    std::error_code error;
    if (fs::exists(path, error)) {
        return true;
    }
    if (error) {
        writer_component().log_print(
            "Unable to inspect text file '{}': {}",
            path.string(), error.message()
        );
        return false;
    }

    std::ofstream output(path, std::ios::app);
    if (!output) {
        writer_component().log_print(
            "Unable to create text file: {}", path.string()
        );
        return false;
    }
    return true;
}

std::wstring window_title(HWND window) {
    std::array<wchar_t, 1024> title {};
    const int length = GetWindowTextW(
        window, title.data(), static_cast<int>(title.size())
    );
    return length > 0
        ? std::wstring {title.data(), static_cast<std::size_t>(length)}
        : std::wstring {};
}

bool is_notepad_window(HWND window) {
    return IsWindowVisible(window) &&
        window_title(window).ends_with(L"Notepad");
}

HWND find_notepad_window(std::wstring_view preferred_title = {}) {
    struct Search {
        std::wstring_view preferred_title;
        HWND any {};
        HWND preferred {};
    } search {preferred_title};

    EnumWindows(
        [](HWND window, LPARAM parameter) -> BOOL {
            auto& state = *reinterpret_cast<Search*>(parameter);
            if (!is_notepad_window(window)) {
                return TRUE;
            }
            if (state.any == nullptr) {
                state.any = window;
            }
            if (!state.preferred_title.empty() &&
                window_title(window).contains(state.preferred_title)) {
                state.preferred = window;
                return FALSE;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&search)
    );
    return search.preferred != nullptr ? search.preferred : search.any;
}

HWND wait_for_notepad_window(const fs::path& path) {
    constexpr auto timeout = std::chrono::seconds {3};
    constexpr auto poll_interval = std::chrono::milliseconds {25};
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    const std::wstring filename = path.filename().wstring();

    do {
        const HWND foreground = GetForegroundWindow();
        if (foreground != nullptr && is_notepad_window(foreground) &&
            window_title(foreground).contains(filename)) {
            return foreground;
        }
        if (const HWND found = find_notepad_window(filename);
            found != nullptr &&
            window_title(found).contains(filename)) {
            return found;
        }
        std::this_thread::sleep_for(poll_interval);
    } while (std::chrono::steady_clock::now() < deadline);

    const HWND foreground = GetForegroundWindow();
    return foreground != nullptr && is_notepad_window(foreground)
        ? foreground
        : find_notepad_window(filename);
}

void focus_notepad_editor(HWND notepad_window) {
    HWND editor_control = nullptr;
    EnumChildWindows(
        notepad_window,
        [](HWND window, LPARAM parameter) -> BOOL {
            std::array<wchar_t, 256> class_name {};
            const int length = GetClassNameW(
                window,
                class_name.data(),
                static_cast<int>(class_name.size())
            );
            if (length <= 0) {
                return TRUE;
            }
            const std::wstring_view value {
                class_name.data(), static_cast<std::size_t>(length)
            };
            if (value == L"Edit" || value.contains(L"RichEdit")) {
                *reinterpret_cast<HWND*>(parameter) = window;
                return FALSE;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&editor_control)
    );
    if (editor_control == nullptr) {
        return;
    }

    const DWORD current_thread = GetCurrentThreadId();
    const DWORD editor_thread =
        GetWindowThreadProcessId(editor_control, nullptr);
    const bool attached = editor_thread != 0 &&
        editor_thread != current_thread &&
        AttachThreadInput(current_thread, editor_thread, TRUE);
    (void)SetFocus(editor_control);
    if (attached) {
        (void)AttachThreadInput(current_thread, editor_thread, FALSE);
    }
}

bool open_path_in_notepad(const fs::path& path) {
    const HWND existing = find_notepad_window();
    if (existing != nullptr && GetForegroundWindow() != existing) {
        (void)ac::taskbar::try_activate_native("notepad");
    }

    const std::wstring arguments = L"\"" + path.wstring() + L"\"";
    SHELLEXECUTEINFOW execution {
        .cbSize = sizeof(SHELLEXECUTEINFOW),
        .fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC,
        .hwnd = nullptr,
        .lpVerb = L"open",
        .lpFile = L"notepad.exe",
        .lpParameters = arguments.c_str(),
        .lpDirectory = path.parent_path().c_str(),
        .nShow = SW_SHOWNORMAL,
    };
    if (!ShellExecuteExW(&execution)) {
        writer_component().log_print(
            "Unable to open text file '{}'. ShellExecute error: {}",
            path.string(), GetLastError()
        );
        return false;
    }
    if (execution.hProcess != nullptr) {
        (void)WaitForInputIdle(execution.hProcess, 1000);
        CloseHandle(execution.hProcess);
    }

    const HWND target = wait_for_notepad_window(path);
    if (target == nullptr) {
        writer_component().log_print(
            "Opened text file '{}', but its Notepad window was not found.",
            path.string()
        );
        return false;
    }
    if (const auto activated = ac::console::activate_window(target);
        !activated) {
        writer_component().log_print(
            "Opened text file '{}', but could not focus Notepad: {}.",
            path.string(),
            ac::console::error_message(activated.error())
        );
        return false;
    }
    focus_notepad_editor(target);
    return true;
}

} // namespace

bool writer_detail::open_in_notepad(const std::filesystem::path& path) {
    return open_path_in_notepad(path);
}

void writer_actions::create_new_note_in_notepad() {
    const auto path = daily_note_path();
    if (!path || !ensure_file_exists(*path)) {
        return;
    }
    (void)open_path_in_notepad(*path);
}
