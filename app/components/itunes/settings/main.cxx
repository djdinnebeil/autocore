import component_settings;
import auto_core.core.shell;

int main() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_settings::DelegatedAction actions[] {
        {
            .label = "Format song",
            .executable = "itunes_formatter.exe",
        },
    };
    return ac::component_settings::run("itunes", actions);
}
