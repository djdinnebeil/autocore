import component_settings;
import auto_core.core.shell;

int main() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_settings::DelegatedAction actions[] {
        {"Discover taskbar applications", "taskbar_builder.exe"}
    };
    return ac::component_settings::run("taskbar", actions);
}
