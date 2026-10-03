import component_star;
import auto_core.core.shell;

int main() {
    ac::shell::set_process_app_user_model_id();
    return ac::component_star::run("logger");
}
