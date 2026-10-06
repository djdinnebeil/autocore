module writer_commands;

import std;
import auto_core.core.paths;
import writer_component;
import writer_notepad;

void writer_actions::launch_task_list() {
    const std::filesystem::path task_list_path =
        ac::paths::writer_directory() / "task_list.txt";

    (void)writer_notepad::open_in_notepad(
        writer_component(),
        task_list_path
    );
}
