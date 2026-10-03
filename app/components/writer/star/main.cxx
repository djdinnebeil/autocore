import std;
import auto_core.core.paths;
import component_star;
import auto_core.core.shell;

namespace {

bool store_missing(const std::filesystem::path& path) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    return error || !present;
}

bool writer_baseline_incomplete() {
    const auto directory = ac::paths::writer_directory();
    return store_missing(directory / "session_prompts.list") ||
        store_missing(directory / "task_list.txt");
}

} // namespace

int main() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_star::DelegatedAction actions[] {
        {
            .label = "Add a new task",
            .executable = "writer_editor.exe",
            .arguments = {"--add-task"},
        },
        {
            .label = "Add a new session prompt",
            .executable = "writer_editor.exe",
            .arguments = {"--add-session-prompt"},
        },
        {
            .label = "Create or open daily note",
            .executable = "writer_editor.exe",
            .arguments = {"--daily-note"},
        },
    };
    return ac::component_star::run(
        "writer",
        actions,
        writer_baseline_incomplete
    );
}
