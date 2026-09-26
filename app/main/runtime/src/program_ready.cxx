module auto_core.main.program_ready;

import std;
import auto_core.main.application;
import auto_core.core.clock;
import auto_core.core.paths;

namespace {

std::string read_task_list_summary() {
    const std::filesystem::path task_list_path =
        ac::paths::writer_directory() / "task_list.txt";
    std::ifstream file(task_list_path, std::ios::binary);
    if (!file) {
        auto_core.log_print(
            "Unable to read task list: {}", task_list_path.string()
        );
        return {};
    }

    std::string task_list;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!task_list.empty()) {
            task_list += '\n';
        }
        task_list += line;
    }
    if (file.bad()) {
        auto_core.log_print(
            "Unable to read complete task list: {}",
            task_list_path.string()
        );
        return {};
    }
    if (task_list.empty()) {
        return "Nothing pending today.";
    }
    return "Today's task list:\n" + task_list;
}

void print_program_ready() {
    std::string message = "Program ready";
    message += "\nToday is " + ac::clock::get_day_of_week();
    if (const std::string task_list = read_task_list_summary();
        !task_list.empty()) {
        message += '\n';
        message += task_list;
    }
    auto_core.print(message);
}

} // namespace

void announce_program_ready() {
    std::thread {print_program_ready}.detach();
}

void print_today_is_day() {
    auto_core.print("Today is {}", ac::clock::get_day_of_week());
}
