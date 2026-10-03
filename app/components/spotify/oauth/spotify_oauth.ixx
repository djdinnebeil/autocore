/**
 * \file spotify_oauth.ixx
 * \brief Spotify OAuth authorization for `spotify_oauth.exe`.
 */
export module spotify_oauth;

import std;

export struct SpotifyOAuthConfig {
    std::string client_id;
    std::string port_number;

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

    /** Runs the browser authorization code flow and saves tokens. */
    SpotifyAuthorizationResult authorize();

private:
    SpotifyOAuthConfig config_;
};
