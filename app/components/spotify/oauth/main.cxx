import std;
import spotify_oauth;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.paths;

ac::Component spotify_oauth_log {"spotify_oauth"};

std::string load_client_id(const std::filesystem::path& config_path) {
    const auto document = ac::ini::read(config_path);
    if (!document) {
        return {};
    }

    const auto client_id = document->find("auth", "client_id");
    if (!client_id) {
        return {};
    }

    return std::string {*client_id};
}

int run_generate_tokens(SpotifyOAuthConfig config) {
    std::cout << "Enter Spotify client ID: ";
    std::getline(std::cin, config.client_id);

    std::cout << "Enter local server port: ";
    std::getline(std::cin, config.port_number);

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

int run_register_devices(SpotifyOAuthConfig config) {
    spotify_oauth_log.log_main("Spotify device registration started");
    SpotifyOAuth oauth(std::move(config));
    const auto result = oauth.register_devices();

    if (!result.succeeded()) {
        spotify_oauth_log.log_print(
            "Spotify device registration failed:\n{}",
            result.message
        );
        return 1;
    }

    spotify_oauth_log.log_print("{}", result.message);
    return 0;
}

int main() {
    spotify_oauth_log.log_main("spotify_oauth.exe started");
    std::cout
        << "spotify_oauth.exe does not write config/spotify.ini. "
           "Run spotify_config.exe if that file is missing.\n";

    SpotifyOAuthConfig config;

    const auto& data_directory = ac::paths::spotify_directory();

    config.token_path = data_directory / "spotify_tokens.ini";
    config.config_path = data_directory / "spotify_codes.ini";

    while (true) {
        std::cout
            << "1. Generate new tokens\n"
            << "2. Retrieve current devices\n"
            << "Select option: ";

        std::string option;
        std::getline(std::cin, option);

        if (option == "1") {
            return run_generate_tokens(std::move(config));
        }

        if (option == "2") {
            config.client_id = load_client_id(config.config_path);

            if (config.client_id.empty()) {
                spotify_oauth_log.log_print(
                    "client_id is missing from {}.\n"
                    "Generate new tokens first, then try again.",
                    config.config_path.filename().string()
                );
                continue;
            }

            return run_register_devices(std::move(config));
        }

        spotify_oauth_log.log_print("Unknown option.");
        return 1;
    }
}
