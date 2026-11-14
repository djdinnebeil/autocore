import component_settings;
import auto_core.core.shell;

namespace ac::wake::settings {

int run() {
    ac::shell::set_process_app_user_model_id();
    return ac::component_settings::run("wake");
}

} // namespace ac::wake::settings
