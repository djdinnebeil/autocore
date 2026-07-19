import std;
import <cpr/cpr.h>;
import <json.hpp>;

namespace {

    constexpr std::string_view currently_playing_url {
        "https://api.spotify.com/v1/me/player/currently-playing"
    };

    bool retrieve_current_song(const std::string& access_token) {
        const cpr::Response response = cpr::Get(
            cpr::Url {currently_playing_url},
            cpr::Header {
                {"Authorization", "Bearer " + access_token}
            }
        );

        if (response.error.code != cpr::ErrorCode::OK) {
            std::cerr
                << "Request failed: "
                << response.error.message
                << '\n';

            return false;
        }

        if (response.status_code == 204) {
            std::cout << "Spotify is not currently playing a track.\n";
            return true;
        }

        if (response.status_code != 200) {
            std::cerr
                << "Failed to retrieve the current track.\n"
                << "Status code: " << response.status_code << '\n'
                << "Response: " << response.text << '\n';

            return false;
        }

        try {
            const auto response_json =
                nlohmann::json::parse(response.text);

            const auto& item = response_json.at("item");

            if (item.is_null()) {
                std::cout << "Spotify returned no current track.\n";
                return true;
            }

            std::cout
                << "Song: "
                << item.at("name").get<std::string>()
                << '\n';

            const auto& artists = item.at("artists");

            if (!artists.empty()) {
                std::cout
                    << "Artist: "
                    << artists.at(0).at("name").get<std::string>()
                    << '\n';
            }

            std::cout
                << "Album: "
                << item.at("album").at("name").get<std::string>()
                << '\n';

            return true;
        }
        catch (const nlohmann::json::exception& error) {
            std::cerr
                << "Unable to interpret Spotify's response: "
                << error.what()
                << '\n'
                << "Response: "
                << response.text
                << '\n';

            return false;
        }
    }
} // namespace

int main() {
    std::cout << "Enter Spotify access token: ";
    std::string access_token;

    std::getline(std::cin, access_token);

    if (access_token.empty()) {
        std::cerr << "No access token was entered.\n";
        return 1;
    }

    const bool success = retrieve_current_song(access_token);

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return success ? 0 : 1;
}