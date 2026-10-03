/**
 * \file spotify_client.cxx
 * \brief Defines Spotify client state and construction.
 */
module spotify_client;

import std;
import auto_core.core.ini;
import auto_core.core.paths;
import spotify_defaults;
import spotify_song_catalog;

HWND spotify_window_hwnd;
Spotify ac_spotify;

Spotify::Spotify() {
    timerate = 55;
    start_timestamp = 0;
    load_devices_from_disk();
    load_tokens_from_disk();
    const auto loaded = spotify::catalog::load_song_format_file(
        ac::paths::spotify_directory() / "song.format"
    );
    song_format = loaded.compiled;
    if (loaded.invalid) {
        song_format_error = loaded.error;
    }
    if (const auto document = ac::ini::read(
            ac::paths::config_directory() / "spotify.ini"
        )) {
        if (const auto value = document->find("spotify", "auto_launch_oauth")) {
            auto_launch_oauth =
                spotify::defaults::auto_launch_enabled(*value);
        }
    }
    content_type = "application/json";
    content_length = "Content-Length: 0";
}
