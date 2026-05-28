module notes;
import base;
import logger;
import clock;
import print;
import main;
import taskbar;
import taskbar_11;
import journey;
import <Windows.h>;

int vs_code_notes_windows;
HWND vs_code_notes_hwnd;
int notepad_notes_windows;
HWND notepad_notes_hwnd;

BOOL CALLBACK enum_vs_code_notes_windows(HWND hwnd, LPARAM lParam) {
    const int buffer_size = 1024;
    WCHAR window_title[buffer_size];
    if (GetWindowTextW(hwnd, window_title, buffer_size) > 0 && IsWindowVisible(hwnd)) {
        wstring title(window_title);
        if (title.length() >= 18 && title.substr(title.length() - 18) == L"Visual Studio Code") {
            vs_code_notes_windows += 1;
            vs_code_notes_hwnd = hwnd;
        }
    }
    return TRUE;
}

void open_in_vs_code(const string& filepath) {
    string command = "start code \"" + filepath + "\"";
    system(command.c_str());
}

void open_in_notepad(const string& filepath) {
    string command = "start notepad \"" + filepath + "\"";
    system(command.c_str());
}

/** \runtime */
void create_new_note_in_vs_code() {
    vs_code_notes_windows = 0;
    EnumWindows(enum_vs_code_notes_windows, 0);
    string filepath = R"(C:\DJ\My Folder\Visual Key\)" + get_datestamp() + ".txt";
    if (fs::exists(filepath) && !vs_code_notes_windows) {
        open_in_vs_code(filepath);
    }
    else if (fs::exists(filepath) && vs_code_notes_windows > 0) {
        HWND current_window = GetForegroundWindow();
        if (current_window != vs_code_notes_hwnd) {
            activate_vs_code();
        }
        open_in_vs_code(filepath);
    }
    else if (!fs::exists(filepath)) {
        ofstream file;
        file.open(filepath, ios::app);
        if (file.is_open()) {
            file.close();
            open_in_vs_code(filepath);
        }
        else {
            print("Error opening the file.");
        }
    }
}

/** \runtime */
void create_new_note_in_notepad() {
    notepad_notes_windows = 0;
    EnumWindows(enum_vs_code_notes_windows, 0);
    string filepath = R"(C:\DJ\My Folder\Auto Sun\)" + get_datestamp() + ".txt";
    if (fs::exists(filepath) && !notepad_notes_windows) {
        open_in_notepad(filepath);
    }
    else if (fs::exists(filepath) && notepad_notes_windows > 0) {
        HWND current_window = GetForegroundWindow();
        if (current_window != notepad_notes_hwnd) {
            activate_notepad();
        }
        open_in_notepad(filepath);
    }
    else if (!fs::exists(filepath)) {
        ofstream file;
        file.open(filepath, ios::app);
        if (file.is_open()) {
            file.close();
            open_in_notepad(filepath);
        }
        else {
            print("Error opening the file.");
        }
    }
}