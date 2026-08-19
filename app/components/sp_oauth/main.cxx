import std;
import sp_oauth;
import sp_oauth_paths;

int main() {
    SpotifyOAuthConfig config;

    config.token_path =
        sp_oauth::paths::star_directory() /
        "sp_tokens.rc";

    std::cout << "Enter Spotify client ID: ";
    std::getline(std::cin, config.client_id);

    std::cout << "Enter Spotify client secret: ";
    std::getline(std::cin, config.client_secret);

    std::cout << "Enter local server port: ";
    std::getline(std::cin, config.port_number);

    SpotifyOAuth oauth(std::move(config));

    const auto result = oauth.authorize();

    if (!result.succeeded()) {
        std::cerr
            << "Spotify authorization failed:\n"
            << result.message
            << '\n';

        return 1;
    }

    std::cout << result.message << '\n';
    return 0;
}
