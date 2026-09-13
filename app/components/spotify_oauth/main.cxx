import std;
import spotify_oauth;
import auto_core.core.ini;
import auto_core.core.paths;

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

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool write_spotify_ini(
    const std::filesystem::path& path,
    const std::string_view directory
) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        std::cerr
            << "Failed to create config directory: "
            << error.message()
            << '\n';
        return false;
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        std::cerr << "Failed to create " << path.string() << '\n';
        return false;
    }

    const std::string contents =
        "[spotify]\ndirectory = " + std::string {directory} + "\n";
    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );
    output.close();
    if (!output) {
        std::cerr << "Failed to write " << path.string() << '\n';
        return false;
    }

    return true;
}

bool ensure_spotify_ini() {
    const auto path = ac::paths::config_directory() / "spotify.ini";

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        return true;
    }
    if (error) {
        std::cerr
            << "Failed to inspect "
            << path.string()
            << ": "
            << error.message()
            << '\n';
        return false;
    }

    std::cout << "Spotify application data directory [.\\spotify]: ";
    std::string input;
    std::getline(std::cin, input);

    const auto directory = trim(input);
    return write_spotify_ini(
        path,
        directory.empty() ? "spotify" : directory
    );
}

int run_generate_tokens(SpotifyOAuthConfig config) {
    std::cout << "Enter Spotify client ID: ";
    std::getline(std::cin, config.client_id);

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

int run_register_devices(SpotifyOAuthConfig config) {
    SpotifyOAuth oauth(std::move(config));
    const auto result = oauth.register_devices();

    if (!result.succeeded()) {
        std::cerr
            << "Spotify device registration failed:\n"
            << result.message
            << '\n';

        return 1;
    }

    std::cout << result.message << '\n';
    return 0;
}

int main() {
    if (!ensure_spotify_ini()) {
        return 1;
    }

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
                std::cerr
                    << "client_id is missing from "
                    << config.config_path.filename().string()
                    << ".\n"
                    << "Generate new tokens first, then try again.\n\n";
                continue;
            }

            return run_register_devices(std::move(config));
        }

        std::cerr << "Unknown option.\n";
        return 1;
    }
}
