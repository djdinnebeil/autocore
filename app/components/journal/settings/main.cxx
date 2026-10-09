import component_settings;
import auto_core.core.shell;

int main() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_settings::DelegatedAction actions[] {
        {
            .label = "Manager journal builder",
            .executable = "journal_builder.exe",
        },
        {
            .label = "Manage series",
            .executable = "journal_series.exe",
        },
        {
            .label = "Configure Firebase",
            .executable = "journal_cloud.exe",
        },
        {
            .label = "Configure extended hours",
            .executable = "journal_clock.exe",
        },
    };
    return ac::component_settings::run("journal", actions);
}
