import std;
import auto_core.core.paths;
import component_settings;
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

namespace ac::writer_app::settings {

int run() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_settings::DelegatedAction actions[] {
        {
            .label = "Add a new task",
            .executable = "writer_ac.exe",
            .arguments = {"--editor", "--add-task"},
        },
        {
            .label = "Add a new session prompt",
            .executable = "writer_ac.exe",
            .arguments = {"--editor", "--add-session-prompt"},
        },
        {
            .label = "Create or open daily note",
            .executable = "writer_ac.exe",
            .arguments = {"--editor", "--daily-note"},
        },
    };
    return ac::component_settings::run(
        "writer",
        actions,
        writer_baseline_incomplete
    );
}

} // namespace ac::writer_app::settings
