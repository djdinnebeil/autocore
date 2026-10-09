import auto_core.core.logging.config;
import component_settings;
import auto_core.core.shell;

int main() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_settings::DelegatedAction actions[] {
        {
            .label = "Manage secrets",
            .executable = "dash_editor.exe",
        },
    };
    return ac::component_settings::run(
        "dash",
        ac::logging::config::LoggingFallback::off,
        actions
    );
}
