module tasks;

import std;
import print;
import ac_component;

import <Windows.h>;

/** \keymap_command */
void launch_task_list() {
    LPCWSTR filePath = LR"(.\link\task_list.rc)";
    ShellExecuteW(NULL, L"open", L"notepad.exe", filePath, NULL, SW_SHOWDEFAULT);
}

std::string get_task_list() {
    const std::string task_list_path = R"(.\link\task_list.rc)";
    std::ifstream file(task_list_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        auto_core.print("error reading file");
        return "";
    }
    std::streamsize size = file.tellg();
    if (size == 0) {
        auto_core.print_and_insert("Nothing pending today.");
        return "";
    }
    file.seekg(0);
    std::string task_list = "Today's task list:";
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        task_list += "\n" + line;
    }
    return task_list;
}

/**
 * \brief Prints the task list.
 *
 * Reads tasks from a file and prints them to the screen. If the file is empty, it indicates
 * that there are no pending tasks for the day.
 * \keymap_command
 */
void print_task_list() {
    auto_core.print_and_insert(get_task_list());
}
