import component_settings;
import auto_core.core.shell;

namespace ac::slash::settings {

int run() {
    ac::shell::set_process_app_user_model_id();
    return ac::component_settings::run("slash");
}

} // namespace ac::slash::settings
