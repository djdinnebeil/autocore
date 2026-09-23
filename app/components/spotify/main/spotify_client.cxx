/**
 * \file spotify_client.cxx
 * \brief Defines Spotify client state and construction.
 */
module spotify_client;

import std;
import auto_core.core.paths;

HWND spotify_window_hwnd;
Spotify ac_spotify;

Spotify::Spotify() {
    timerate = 55;
    start_timestamp = 0;
    tokens_path = ac::paths::spotify_directory() / "spotify_tokens.ini";
    config_path = ac::paths::spotify_directory() / "spotify_codes.ini";
    content_type = "application/json";
    content_length = "Content-Length: 0";
}
