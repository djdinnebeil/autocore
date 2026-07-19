export module sp_oauth;

import std;

export struct SpotifyOAuthConfig {
    std::string client_id;
    std::string client_secret;
    std::string port_number;

    std::filesystem::path token_path {
        R"(.\star\sp_tokens.rc)"
    };

    bool is_valid() const noexcept;
};

export enum class SpotifyAuthorizationStatus {
    success,
    invalid_configuration,
    server_start_failed,
    browser_open_failed,
    authorization_denied,
    invalid_callback,
    token_exchange_failed,
    token_save_failed
};

export struct SpotifyAuthorizationResult {
    SpotifyAuthorizationStatus status;
    std::string message;

    bool succeeded() const noexcept;
};

export class SpotifyOAuth {
public:
    explicit SpotifyOAuth(SpotifyOAuthConfig config);

    SpotifyAuthorizationResult authorize();

private:
    SpotifyOAuthConfig config_;
};