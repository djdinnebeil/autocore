module tasks;

import std;
import auto_core.paths;
import ac_main;

import <Windows.h>;

/** \keymap_command */
void launch_task_list() {
    const std::filesystem::path task_list_path =
        ac::paths::link_directory() / "task_list.rc";

    const HINSTANCE result = ShellExecuteW(
        nullptr,
        L"open",
        L"notepad.exe",
        task_list_path.c_str(),
        ac::paths::link_directory().c_str(),
        SW_SHOWDEFAULT
    );

    if (reinterpret_cast<std::intptr_t>(result) <= 32) {
        auto_core.logg_and_print(
            "Unable to open task list '{}'. ShellExecute error: {}",
            task_list_path,
            reinterpret_cast<std::intptr_t>(result)
        );
    }
}

std::string get_task_list() {
    const std::filesystem::path task_list_path =
        ac::paths::link_directory() / "task_list.rc";

    std::ifstream file(
        task_list_path,
        std::ios::binary | std::ios::ate
    );

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

void tasks::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("launch_task_list", &::launch_task_list);
    registry.add("print_task_list", &::print_task_list);
}
