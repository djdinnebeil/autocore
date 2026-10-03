import component_star;
import auto_core.core.shell;

int main() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_star::DelegatedAction actions[] {
        {"Discover taskbar applications", "taskbar_builder.exe"}
    };
    return ac::component_star::run("taskbar", actions);
}
