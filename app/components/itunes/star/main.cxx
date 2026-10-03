import component_star;
import auto_core.core.shell;

int main() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_star::DelegatedAction actions[] {
        {
            .label = "Format song",
            .executable = "itunes_formatter.exe",
        },
    };
    return ac::component_star::run("itunes", actions);
}
