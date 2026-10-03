import auto_core.core.logging.config;
import component_star;
import auto_core.core.shell;

int main() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_star::DelegatedAction actions[] {
        {
            .label = "Manage secrets",
            .executable = "dash_editor.exe",
        },
    };
    return ac::component_star::run(
        "dash",
        ac::logging::config::LoggingFallback::off,
        actions
    );
}
