import std;
import sp_oauth;

int main() {
    SpotifyOAuthConfig config;

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