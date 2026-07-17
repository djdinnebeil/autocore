import std;
import <cpr/cpr.h>;
import <json.hpp>;
import <CivetServer.h>;

std::string client_id;
std::string client_secret;
std::string port_number;
std::string redirect_uri;
std::string account_url = "https://accounts.spotify.com";
std::string scope = "user-follow-read%20ugc-image-upload%20user-read-playback-state%20user-modify-playback-state%20user-read-currently-playing%20user-read-private%20user-read-email%20user-follow-modify%20user-follow-read%20user-library-modify%20user-library-read%20streaming%20app-remote-control%20user-read-playback-position%20user-top-read%20user-read-recently-played%20playlist-modify-private%20playlist-read-collaborative%20playlist-read-private%20playlist-modify-public";
std::string authorization_link;
std::string authorization_code;
std::string access_token;
std::string refresh_token;

using namespace cpr;

// Atomic flag to signal the server thread to terminate
std::atomic<bool> terminateServer(false);

// Encode to Base64
std::string base64_encode(const std::string& in) {
    std::string out;
    int val = 0, valb = -6;
    for (uint8_t c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

void saveTokens() {
    std::ofstream outFile("sp_tokens.rc");

    if (!outFile.is_open()) {
        std::cerr << "Unable to open token file.\n";
        return;
    }

    const time_t timestamp =
        std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()
        );

    outFile << access_token << '\n'
        << refresh_token << '\n'
        << timestamp;

    std::cout << "Tokens saved to sp_tokens.rc\n";
}
// Callback handler for handling Spotify authorization
class SpotifyCallbackHandler : public CivetHandler {
public:
    bool handleGet(CivetServer* server, mg_connection* conn) {
        const mg_request_info* req_info = mg_get_request_info(conn);
        std::string query = req_info->query_string ? req_info->query_string : "";
        size_t code_pos = query.find("code=");
        if (code_pos != std::string::npos) {
            authorization_code = query.substr(code_pos + 5);
            std::cout << "Authorization code received: " << authorization_code << "\n";

            std::string credentials = client_id + ":" + client_secret;
            std::string encoded_credentials = base64_encode(credentials);
            cpr::Response r = cpr::Post(cpr::Url{"https://accounts.spotify.com/api/token"},
                cpr::Header{{"Content-Type", "application/x-www-form-urlencoded"},
                            {"Authorization", "Basic " + encoded_credentials}},
                cpr::Payload{{"grant_type", "authorization_code"},
                             {"code", authorization_code},
                             {"redirect_uri", redirect_uri}});
            if (r.status_code == 200) {
                auto response_json = nlohmann::json::parse(r.text);
                access_token = response_json["access_token"];
                refresh_token = response_json["refresh_token"];
                saveTokens();
                retrieveCurrentSong();
            }
            else {
                std::cout << "Failed to retrieve tokens. Status Code: " << r.status_code << "\nResponse: " << r.text << "\n";
            }

            terminateServer.store(true);
            mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nAuthorization successful. You can close this page.");
            return true;
        }
        return false;
    }

    void retrieveCurrentSong() {
        cpr::Response r = cpr::Get(cpr::Url{"https://api.spotify.com/v1/me/player/currently-playing"},
            cpr::Header{{"Authorization", "Bearer " + access_token}});
        if (r.status_code == 200) {
            auto response_json = nlohmann::json::parse(r.text);
            std::cout << "Song Name: " << response_json["item"]["name"] << "\n";
            std::cout << "Artist: " << response_json["item"]["artists"][0]["name"] << "\n";
            std::cout << "Album: " << response_json["item"]["album"]["name"] << "\n";
        }
        else {
            std::cout << "Failed to retrieve current song. Status Code: " << r.status_code << "\nResponse: " << r.text << "\n";
        }
    }
};

void runServer() {
    const char* options[] = {
        "listening_ports",
        port_number.c_str(),
        "document_root",
        ".",
        nullptr
    };

    CivetServer server(options);
    SpotifyCallbackHandler spotifyCallbackHandler;
    server.addHandler("/callback", spotifyCallbackHandler);

    while (!terminateServer.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void set_authorization_link() {
    authorization_link = account_url + "/authorize?client_id=" + client_id + "&response_type=code&redirect_uri=" + redirect_uri + "&scope=" + scope;
}

int main() {
    std::cout << "Enter Spotify client ID: ";
    getline(std::cin, client_id);

    std::cout << "Enter Spotify client secret: ";
    getline(std::cin, client_secret);

    std::cout << "Enter local server port: ";
    getline(std::cin, port_number);

    redirect_uri =
        "http://127.0.0.1:" + port_number + "/callback";

    if (client_id.empty() || client_secret.empty() || port_number.empty()) {
        std::cerr << "Client ID, client secret, and port number cannot be empty.\n";
        return 1;
    }

    std::thread serverThread(runServer);
    std::cout << "Server started on port " << port_number << std::endl;
    set_authorization_link();
    std::cout << "Authorization link:\n" << authorization_link << "\n";
    std::cin.get();
    serverThread.join(); // Properly join the thread before exiting
    return 0;
}
