module writer_commands;

import std;
import auto_core.core.paths;

void writer_actions::launch_task_list() {
    const std::filesystem::path task_list_path =
        ac::paths::writer_directory() / "task_list.txt";

    (void)writer_detail::open_in_notepad(task_list_path);
}
