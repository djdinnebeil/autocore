import component_settings;
import auto_core.core.shell;

namespace ac::itunes_app::settings {

int run() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_settings::DelegatedAction actions[] {
        {
            .label = "Format song",
            .executable = "itunes_ac.exe",
            .arguments = {"--formatter"},
        },
    };
    return ac::component_settings::run("itunes", actions);
}

} // namespace ac::itunes_app::settings
