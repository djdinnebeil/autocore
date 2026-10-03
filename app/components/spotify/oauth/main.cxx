import std;
import spotify_oauth;
import spotify_application_data;
import auto_core.core.component;
import auto_core.core.logging.config;

import <Windows.h>;
import auto_core.core.shell;

ac::Component spotify_oauth_log {
    "spotify_oauth",
    ac::logging::config::LoggingScope {"spotify"}
};

BOOL WINAPI oauth_console_control(DWORD event) {
    switch (event) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            ::ExitProcess(2);
            return TRUE;
        default:
            return FALSE;
    }
}

int run_authorize() {
    const auto client_id = spotify::data::load_client_id();
    if (!client_id) {
        spotify_oauth_log.log_print(
            "Spotify authorization failed. The client ID is not configured."
        );
        return 1;
    }

    SpotifyOAuthConfig config;
    config.client_id = *client_id;
    std::cout << "Enter local server port: ";
    if (!std::getline(std::cin, config.port_number)) {
        return 2;
    }

    spotify_oauth_log.log_main("Spotify authorization started");
    SpotifyOAuth oauth(std::move(config));
    const auto result = oauth.authorize();

    if (!result.succeeded()) {
        spotify_oauth_log.log_print(
            "Spotify authorization failed:\n{}",
            result.message
        );
        return 1;
    }

    spotify_oauth_log.log_print("{}", result.message);
    return 0;
}

int main() {
    ac::shell::set_process_app_user_model_id();
    ::SetConsoleCtrlHandler(oauth_console_control, TRUE);
    spotify_oauth_log.log_main("spotify_oauth.exe started");

    while (true) {
        std::cout
            << "spotify_oauth\n"
            << "\n"
            << "1. Authorize Spotify\n"
            << "2. Abort\n"
            << "Select option: ";

        std::string option;
        if (!std::getline(std::cin, option)) {
            return 2;
        }
        if (option == "1") {
            return run_authorize();
        }
        if (option == "2") {
            return 2;
        }
        spotify_oauth_log.log_print("Unknown option.");
    }
}
