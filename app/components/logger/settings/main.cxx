import component_settings;
import auto_core.core.shell;

namespace ac::logger::runtime {
    void refresh_main_log();
}

namespace {

int run_menu() {
    const ac::component_settings::LocalAction actions[] {
        {"Update Main log", ac::logger::runtime::refresh_main_log}
    };
    return ac::component_settings::run("logger", {}, nullptr, {}, actions);
}

} // namespace

namespace ac::logger::settings {

int run() {
    ac::shell::set_process_app_user_model_id();
    return run_menu();
}

int run_after_refresh() {
    ac::shell::set_process_app_user_model_id();
    ac::logger::runtime::refresh_main_log();
    return run_menu();
}

} // namespace ac::logger::settings
