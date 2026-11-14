import component_settings;
import auto_core.core.shell;

namespace ac::taskbar_app::settings {

int run() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_settings::DelegatedAction actions[] {
        {
            .label = "Discover taskbar applications",
            .executable = "taskbar_ac.exe",
            .arguments = {"--builder"},
        },
    };
    return ac::component_settings::run("taskbar", actions);
}

} // namespace ac::taskbar_app::settings
