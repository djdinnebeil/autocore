import component_settings;
import auto_core.core.shell;

import <Windows.h>;

int main() {
    ac::shell::set_process_app_user_model_id();
    const ac::component_settings::DelegatedAction actions[] {
        {"Manage client and devices", "spotify_editor.exe"},
        {"Authorize Spotify", "spotify_oauth.exe", {}, CREATE_NEW_CONSOLE},
        {"Format song", "spotify_formatter.exe"}
    };
    return ac::component_settings::run("spotify", actions);
}
