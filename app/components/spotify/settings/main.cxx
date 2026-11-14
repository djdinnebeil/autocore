import component_settings;
import auto_core.core.shell;

import <Windows.h>;

namespace ac::spotify_app::settings {

int run() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_settings::DelegatedAction actions[] {
        {
            .label = "Manage client and devices",
            .executable = "spotify_ac.exe",
            .arguments = {"--editor"},
        },
        {
            .label = "Authorize Spotify",
            .executable = "spotify_ac.exe",
            .arguments = {"--oauth"},
            .creation_flags = CREATE_NEW_CONSOLE,
        },
        {
            .label = "Format song",
            .executable = "spotify_ac.exe",
            .arguments = {"--formatter"},
        },
    };
    return ac::component_settings::run("spotify", actions);
}

} // namespace ac::spotify_app::settings
