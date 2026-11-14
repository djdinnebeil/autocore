import component_settings;
import auto_core.core.shell;

namespace ac::journal_app::settings {

int run() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_settings::DelegatedAction actions[] {
        {
            .label = "Manager journal builder",
            .executable = "journal_ac.exe",
            .arguments = {"--builder"},
        },
        {
            .label = "Manage series",
            .executable = "journal_ac.exe",
            .arguments = {"--series"},
        },
        {
            .label = "Configure Firebase",
            .executable = "journal_ac.exe",
            .arguments = {"--cloud"},
        },
        {
            .label = "Configure extended hours",
            .executable = "journal_ac.exe",
            .arguments = {"--clock"},
        },
    };
    return ac::component_settings::run("journal", actions);
}

} // namespace ac::journal_app::settings
