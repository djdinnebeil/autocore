module sp_oauth;

import std;
import <Windows.h>;

#pragma comment(lib, "Shell32.lib")

import <cpr/cpr.h>;
import <json.hpp>;
import <CivetServer.h>;


namespace {

    constexpr std::string_view account_url {
        "https://accounts.spotify.com"
    };

    constexpr std::string_view token_endpoint {
        "https://accounts.spotify.com/api/token"
    };

    constexpr std::string_view callback_path {
        "/callback"
    };

    const std::array spotify_scopes {
        std::string_view {"user-follow-read"},
        std::string_view {"ugc-image-upload"},
        std::string_view {"user-read-playback-state"},
        std::string_view {"user-modify-playback-state"},
        std::string_view {"user-read-currently-playing"},
        std::string_view {"user-read-private"},
        std::string_view {"user-read-email"},
        std::string_view {"user-follow-modify"},
        std::string_view {"user-library-modify"},
        std::string_view {"user-library-read"},
        std::string_view {"streaming"},
        std::string_view {"app-remote-control"},
        std::string_view {"user-read-playback-position"},
        std::string_view {"user-top-read"},
        std::string_view {"user-read-recently-played"},
        std::string_view {"playlist-modify-private"},
        std::string_view {"playlist-read-collaborative"},
        std::string_view {"playlist-read-private"},
        std::string_view {"playlist-modify-public"}
    };

    struct SpotifyAuthorizationSession {
        std::string redirect_uri;
        std::string access_token;
        std::string refresh_token;

        SpotifyAuthorizationStatus status {
            SpotifyAuthorizationStatus::invalid_callback
        };

        std::string message;

        std::atomic<bool> complete {false};
    };

    std::string join_scopes() {
        std::string result;

        for (const std::string_view scope : spotify_scopes) {
            if (!result.empty()) {
                result += ' ';
            }

            result += scope;
        }

        return result;
    }

    std::string base64_encode(std::string_view input) {
        constexpr std::string_view characters {
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/"
        };

        std::string output;

        int value = 0;
        int bit_position = -6;

        for (const unsigned char character : input) {
            value = (value << 8) + character;
            bit_position += 8;

            while (bit_position >= 0) {
                output.push_back(
                    characters[
                        (value >> bit_position) & 0x3F
                    ]
                );

                bit_position -= 6;
            }
        }

        if (bit_position > -6) {
            output.push_back(
                characters[
                    ((value << 8) >> (bit_position + 8)) & 0x3F
                ]
            );
        }

        while (output.size() % 4 != 0) {
            output.push_back('=');
        }

        return output;
    }

    std::optional<std::string> get_query_parameter(
        std::string_view query,
        std::string_view parameter
    ) {
        const std::string key =
            std::string(parameter) + '=';

        const std::size_t key_position =
            query.find(key);

        if (key_position == std::string_view::npos) {
            return std::nullopt;
        }

        const std::size_t value_start =
            key_position + key.size();

        const std::size_t value_end =
            query.find('&', value_start);

        return std::string {
            query.substr(
                value_start,
                value_end == std::string_view::npos
                    ? std::string_view::npos
                    : value_end - value_start
            )
        };
    }

    std::string build_redirect_uri(
        const SpotifyOAuthConfig& config
    ) {
        return
            "http://127.0.0.1:"
            + config.port_number
            + std::string(callback_path);
    }

    std::string build_authorization_link(
        const SpotifyOAuthConfig& config,
        std::string_view redirect_uri
    ) {
        const cpr::Parameters parameters {
            {"client_id", config.client_id},
            {"response_type", "code"},
            {"redirect_uri", std::string(redirect_uri)},
            {"scope", join_scopes()}
        };

        const cpr::CurlHolder curl_holder;

        return std::string(account_url)
            + "/authorize?"
            + parameters.GetContent(curl_holder);
    }

    bool open_authorization_link(
        const std::string& authorization_link
    ) {
        const std::wstring wide_link {
            authorization_link.begin(),
            authorization_link.end()
        };

        const HINSTANCE result = ShellExecuteW(
            nullptr,
            L"open",
            wide_link.c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL
        );

        return reinterpret_cast<std::intptr_t>(result) > 32;
    }

    bool save_tokens(
        const SpotifyOAuthConfig& config,
        const SpotifyAuthorizationSession& session,
        std::string& error_message
    ) {
        std::ofstream output_file(config.token_path);

        if (!output_file.is_open()) {
            error_message =
                "Unable to open token file: "
                + config.token_path.string();

            return false;
        }

        const auto authorization_time =
            std::chrono::system_clock::now();

        const std::time_t authorization_timestamp =
            std::chrono::system_clock::to_time_t(
                authorization_time
            );

        const std::time_t refresh_token_expiration_timestamp =
            std::chrono::system_clock::to_time_t(
                authorization_time + std::chrono::days(179)
            );

        output_file
            << session.access_token << '\n'
            << session.refresh_token << '\n'
            << authorization_timestamp << '\n'
            << refresh_token_expiration_timestamp;

        if (!output_file) {
            error_message =
                "An error occurred while writing the token file.";

            return false;
        }

        return true;
    }

    bool exchange_authorization_code(
        const SpotifyOAuthConfig& config,
        SpotifyAuthorizationSession& session,
        std::string_view authorization_code,
        std::string& error_message
    ) {
        const std::string credentials =
            config.client_id + ":" + config.client_secret;

        const std::string encoded_credentials =
            base64_encode(credentials);

        const cpr::Response response = cpr::Post(
            cpr::Url {
                std::string(token_endpoint)
            },
            cpr::Header {
                {
                    "Content-Type",
                    "application/x-www-form-urlencoded"
                },
                {
                    "Authorization",
                    "Basic " + encoded_credentials
                }
            },
            cpr::Payload {
                {
                    "grant_type",
                    "authorization_code"
                },
                {
                    "code",
                    std::string(authorization_code)
                },
                {
                    "redirect_uri",
                    session.redirect_uri
                }
            }
        );

        if (response.status_code != 200) {
            error_message =
                "Spotify token request failed with status code "
                + std::to_string(response.status_code)
                + ". Response: "
                + response.text;

            return false;
        }

        try {
            const nlohmann::json response_json =
                nlohmann::json::parse(response.text);

            if (!response_json.contains("access_token")
                || !response_json["access_token"].is_string()) {
                error_message =
                    "Spotify did not return a valid access token.";

                return false;
            }

            if (!response_json.contains("refresh_token")
                || !response_json["refresh_token"].is_string()) {
                error_message =
                    "Spotify did not return a valid refresh token.";

                return false;
            }

            session.access_token =
                response_json["access_token"].get<std::string>();

            session.refresh_token =
                response_json["refresh_token"].get<std::string>();
        }
        catch (const nlohmann::json::exception& exception) {
            error_message =
                "Unable to parse Spotify token response: "
                + std::string(exception.what());

            return false;
        }

        return true;
    }

    void send_browser_response(
        mg_connection* connection,
        int status_code,
        std::string_view status_text,
        std::string_view message
    ) {
        mg_printf(
            connection,
            "HTTP/1.1 %d %.*s\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%.*s",
            status_code,
            static_cast<int>(status_text.size()),
            status_text.data(),
            static_cast<int>(message.size()),
            message.data()
        );
    }

    class SpotifyCallbackHandler final : public CivetHandler {
    public:
        SpotifyCallbackHandler(
            const SpotifyOAuthConfig& config,
            SpotifyAuthorizationSession& session
        )
            : config_(config),
            session_(session) {
        }

        bool handleGet(
            CivetServer*,
            mg_connection* connection
        ) override {
            const mg_request_info* request_info =
                mg_get_request_info(connection);

            const std::string_view query =
                request_info->query_string
                ? request_info->query_string
                : "";

            if (const auto error =
                get_query_parameter(query, "error")) {
                session_.status =
                    SpotifyAuthorizationStatus::authorization_denied;

                session_.message =
                    "Spotify authorization was denied: " + *error;

                send_browser_response(
                    connection,
                    200,
                    "OK",
                    "Spotify authorization was not completed."
                );

                finish();
                return true;
            }

            const auto authorization_code =
                get_query_parameter(query, "code");

            if (!authorization_code) {
                session_.status =
                    SpotifyAuthorizationStatus::invalid_callback;

                session_.message =
                    "The Spotify callback did not contain "
                    "an authorization code.";

                send_browser_response(
                    connection,
                    400,
                    "Bad Request",
                    "The authorization callback was invalid."
                );

                finish();
                return true;
            }

            std::cout << "Authorization code received.\n";

            std::string error_message;

            if (!exchange_authorization_code(
                config_,
                session_,
                *authorization_code,
                error_message
            )) {
                session_.status =
                    SpotifyAuthorizationStatus::token_exchange_failed;

                session_.message =
                    std::move(error_message);

                send_browser_response(
                    connection,
                    502,
                    "Bad Gateway",
                    "Spotify authorization failed while "
                    "retrieving tokens."
                );

                finish();
                return true;
            }

            if (!save_tokens(
                config_,
                session_,
                error_message
            )) {
                session_.status =
                    SpotifyAuthorizationStatus::token_save_failed;

                session_.message =
                    std::move(error_message);

                send_browser_response(
                    connection,
                    500,
                    "Internal Server Error",
                    "Authorization succeeded, but the tokens "
                    "could not be saved."
                );

                finish();
                return true;
            }

            session_.status =
                SpotifyAuthorizationStatus::success;

            session_.message =
                "Spotify authorization completed successfully.";

            send_browser_response(
                connection,
                200,
                "OK",
                "Authorization successful. "
                "You can close this page."
            );

            finish();
            return true;
        }

    private:
        const SpotifyOAuthConfig& config_;
        SpotifyAuthorizationSession& session_;

        void finish() {
            session_.complete.store(
                true,
                std::memory_order_release
            );
        }
    };

} // namespace

bool SpotifyOAuthConfig::is_valid() const noexcept {
    return !client_id.empty()
        && !client_secret.empty()
        && !port_number.empty();
}

bool SpotifyAuthorizationResult::succeeded() const noexcept {
    return status == SpotifyAuthorizationStatus::success;
}

SpotifyOAuth::SpotifyOAuth(
    SpotifyOAuthConfig config
)
    : config_(std::move(config)) {
}

SpotifyAuthorizationResult SpotifyOAuth::authorize() {
    if (!config_.is_valid()) {
        return {
            SpotifyAuthorizationStatus::invalid_configuration,
            "Client ID, client secret, and port number "
            "cannot be empty."
        };
    }

    SpotifyAuthorizationSession session;
    session.redirect_uri =
        build_redirect_uri(config_);

    const std::string authorization_link =
        build_authorization_link(
            config_,
            session.redirect_uri
        );

    try {
        const char* server_options[] {
            "listening_ports",
            config_.port_number.c_str(),
            "document_root",
            ".",
            nullptr
        };

        CivetServer server(server_options);

        SpotifyCallbackHandler callback_handler(
            config_,
            session
        );

        server.addHandler(
            callback_path.data(),
            callback_handler
        );

        std::cout
            << "Server started on port "
            << config_.port_number
            << ".\n";

        if (!open_authorization_link(authorization_link)) {
            return {
                SpotifyAuthorizationStatus::browser_open_failed,
                "Unable to open the Spotify authorization link."
            };
        }

        std::cout
            << "Authorization link:\n"
            << authorization_link
            << '\n';

        while (!session.complete.load(
            std::memory_order_acquire
        )) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(100)
            );
        }
    }
    catch (const std::exception& exception) {
        return {
            SpotifyAuthorizationStatus::server_start_failed,
            "Unable to run the local authorization server: "
                + std::string(exception.what())
        };
    }

    return {
        session.status,
        std::move(session.message)
    };
}