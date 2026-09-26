/**
\file spotify_playback.cxx
\brief Implements Spotify playback and device control.
*/
module spotify_client;

import std;
import auto_core.core.thread;
import spotify_component;
import spotify_http;
import <json.hpp>;
import <cpr/cpr.h>;
import <chrono>;

using std::stoll;
using namespace cpr;

json parse(const std::string& s);
void start_spotify_desktop_playback();

/**
 * \brief Upserts Spotify Connect devices into [devices] using Spotify's names.
 */
void Spotify::update_devices() {
    std::string url = "https://api.spotify.com/v1/me/player/devices";
    auto response = Get(
        Url {url},
        Header {{"Authorization", authorization_header},{"Content-Type", content_type}}
    );
    bool devices_changed = false;
    auto devices_json = parse(response.text);
    if (!devices_json.contains("devices") || !devices_json["devices"].is_array()) {
        return;
    }

    for (const auto& device : devices_json["devices"]) {
        if (!device.contains("name") || !device.contains("id")) {
            continue;
        }
        if (!device["name"].is_string() || !device["id"].is_string()) {
            continue;
        }

        const std::string device_name =
            ascii_lower(device["name"].get<std::string>());
        const std::string device_id = device["id"].get<std::string>();

        bool found = false;
        for (auto& [name, id] : devices) {
            if (device_name_equals_ignore_case(name, device_name)) {
                if (name != device_name) {
                    name = device_name;
                    devices_changed = true;
                }
                if (id != device_id) {
                    id = device_id;
                    devices_changed = true;
                }
                found = true;
                break;
            }
        }

        if (!found) {
            devices.emplace_back(device_name, device_id);
            devices_changed = true;
        }
    }

    if (devices_changed) {
        save_config();
    }
}

/**
 * \brief Pauses the current Spotify song.
 * \return Status code of the pause request.
 */
int Spotify::pause_song() {
    if (!refresh_tokens()) {
        return 429;
    }
    auto response = Put(
        Url {"https://api.spotify.com/v1/me/player/pause"},
        Header {
            {"Authorization", "Bearer " + access_token},
            {"Content-Type", "application/json"},
            {"Content-Length", "0"}
        });
    if (response.status_code != 204) {
        spotify_component.log_print("Error: Status Code {} - {}", response.status_code, response.text);
    }
    return response.status_code;
}
/**
 * \brief Transfers playback to the specified device.
 * \param device_id The device ID to transfer playback to.
 */
void Spotify::transfer_playback(std::string device_id) {
    if (!refresh_tokens()) {
        return;
    }
    std::string url = "https://api.spotify.com/v1/me/player";
    std::string body = format(R"({{"device_ids": ["{}"], "play": true}})", device_id);
    auto response = Put(
        Url {url},
        Header {{"Authorization", authorization_header},{"Content-Type", content_type}},
        Body {body}
    );
    if (response.status_code != 204) {
        spotify_component.log_print("Error: Status Code {} - {}", response.status_code, response.text);
    }
}
/**
 * \brief Starts playback on this computer's configured device.
 */
void Spotify::start_playback_on_desktop() {
    if (!refresh_tokens()) {
        return;
    }

    update_devices();

    const std::string device_id = first_configured_device_id();
    if (device_id.empty()) {
        spotify_component.log_print(
            "No configured Spotify device is available for playback."
        );
        return;
    }

    transfer_playback(device_id);
}
/**
 * \brief Switches playback to the next configured device that has an ID.
 */
void Spotify::switch_player() {
    if (!refresh_tokens()) {
        return;
    }
    update_devices();
    std::string url = "https://api.spotify.com/v1/me/player";
    auto response = Get(
        Url {url},
        Header {{"Authorization", authorization_header},{"Content-Type", content_type}}
    );
    if (response.status_code == 204) {
        play_pause();
        return;
    }
    if (response.status_code != 200) {
        spotify_component.log_print("Error: Status Code {} - {}", response.status_code, response.text);
        return;
    }
    auto playback_details = parse(response.text);
    std::string current_device_id = playback_details["device"]["id"];

    std::vector<std::string> configured_ids;
    for (const auto& [name, id] : devices) {
        if (!id.empty()) {
            configured_ids.push_back(id);
        }
    }

    if (configured_ids.empty()) {
        spotify_component.log_print("No configured Spotify devices have IDs.");
        return;
    }

    const auto current = std::find(
        configured_ids.begin(),
        configured_ids.end(),
        current_device_id
    );

    if (current == configured_ids.end()) {
        spotify_component.log_print("Device not added");
        return;
    }

    if (configured_ids.size() == 1) {
        spotify_component.log_print("No other configured Spotify device.");
        return;
    }

    auto next = std::next(current);
    if (next == configured_ids.end()) {
        next = configured_ids.begin();
    }

    transfer_playback(*next);
}
/**
 * \brief Plays the current Spotify song.
 * \return Status code of the play request.
 */
int Spotify::play_song() {
    if (!refresh_tokens()) {
        return 429;
    }
    auto response = Put(
        Url {"https://api.spotify.com/v1/me/player/play"},
        Header {
            {"Authorization", authorization_header},
            {"Content-Type", "application/json"},
            {"Content-Length", "0"},
        }
    );
    if (response.status_code == 404) {
        spotify_component.log_print("no active device");
    }
    else if (response.status_code != 204) {
        spotify_component.log_print("Error: Status Code {} - {}", response.status_code, response.text);
    }
    return response.status_code;
}
/**
 * \brief Checks if Spotify is currently playing a song.
 * \return True if a song is playing, false otherwise.
 */
bool Spotify::is_spotify_playing() {
    if (!refresh_tokens()) {
        return false;
    }
    std::string url = "https://api.spotify.com/v1/me/player/currently-playing";
    auto response = Get(
        Url {url},
        Header {{"Authorization", authorization_header},{"Content-Type", content_type}}
    );
    if (response.status_code != 200) {
        return false;
    }
    auto playback_details = parse(response.text);
    if (!playback_details.contains("is_playing")) {
        return false;
    }
    bool is_playing = playback_details["is_playing"];
    return is_playing;
}
/**
 * \brief Plays or pauses the Spotify playback.
 */
void Spotify::play_pause() {
    if (!refresh_tokens()) {
        spotify_component.log_print("tokens not refreshed in Spotify::play_pause()");
        return;
    }
    if (is_spotify_playing()) {
        spotify_component.log_main("is_spotify_playing() == true");
        pause_song();
        return;
    }
    if (play_song() == 204) {
        spotify_component.log_main("play_song() == 204");
        return;
    }
    if (is_spotify_open()) {
        spotify_component.log_main("is_spotify_open() == true");
        start_playback_on_desktop();
    }
    else {
        spotify_component.log_main("starting Spotify");
        start_spotify_desktop_playback();
    }
}
/**
 * \brief Sends a POST request to skip to the next or previous song.
 * \param url URL for the POST request.
 */
void Spotify::post_next_or_prev(std::string url) {
    if (!refresh_tokens()) {
        return;
    }
    auto response = Post(
        Url {url},
        Header {{"Authorization", authorization_header},{"Content-Type", content_type}}
    );
    if (!spotify_http::is_success_status(response.status_code)) {
        spotify_component.log_print("post_next_or_prev() - Error: Status Code {} - {}", response.status_code, response.text);
    }
}
/**
 * \brief Skips to the next Spotify song.
 */
void Spotify::next_song() {
    std::string url = "https://api.spotify.com/v1/me/player/next";
    post_next_or_prev(url);
}
/**
 * \brief Skips to the previous Spotify song.
 */
void Spotify::prev_song() {
    std::string url = "https://api.spotify.com/v1/me/player/previous";
    post_next_or_prev(url);
}
