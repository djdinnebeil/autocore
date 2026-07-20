/**
\file sp_playback.cxx
\brief Implements Spotify playback and device control.
*/
module sp_c;

import visual;
import thread;
import sp_x;
import <json.hpp>;
import <cpr/cpr.h>;
import <chrono>;

using std::stoll;
using namespace cpr;

json parse(const string& s);
void start_playback_sp_thread();

/**
 * \brief Retrieves Spotify devices.
 */
void Spotify::get_devices() {
    ifstream rc(devices_path);
    getline(rc, desktop_device_id);
    getline(rc, mobile_device_id);
    rc.close();
}
/**
 * \brief Updates Spotify devices.
 */
void Spotify::update_devices() {
    string url = "https://api.spotify.com/v1/me/player/devices";
    auto response = Get(
        Url {url},
        Header {{"Authorization", authorization_header},{"Content-Type", content_type}}
    );
    bool device_id_changed = false;
    auto devices = parse(response.text);
    for (const auto& device : devices["devices"]) {
        if (device.contains("name") && device.contains("id")) {
            string device_name = device["name"].get<string>();
            string device_id = device["id"].get<string>();

            if (device_name == "DESKTOP-DNP5C1N" && desktop_device_id != device_id) {
                desktop_device_id = device_id;
                device_id_changed = true;
            }
            else if (device_name == "iPhone" && mobile_device_id != device_id) {
                mobile_device_id = device_id;
                device_id_changed = true;
            }
        }
    }
    if (device_id_changed) {
        ofstream rc(devices_path);
        if (rc.is_open()) {
            oss os;
            os << desktop_device_id << "\n" << mobile_device_id;
            rc << os.str();
            rc.close();
        }
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
        sp_logger.logg_and_print("Error: Status Code {} - {}", response.status_code, response.text);
    }
    return response.status_code;
}
/**
 * \brief Transfers playback to the specified device.
 * \param device_id The device ID to transfer playback to.
 */
void Spotify::transfer_playback(string device_id) {
    if (!refresh_tokens()) {
        return;
    }
    string url = "https://api.spotify.com/v1/me/player";
    string body = format(R"({{"device_ids": ["{}"], "play": true}})", device_id);
    auto response = Put(
        Url {url},
        Header {{"Authorization", authorization_header},{"Content-Type", content_type}},
        Body {body}
    );
    if (response.status_code != 204) {
        sp_logger.logg_and_print("Error: Status Code {} - {}", response.status_code, response.text);
    }
}
/**
 * \brief Starts playback on the desktop device.
 */
void Spotify::start_playback_on_desktop() {
    transfer_playback(desktop_device_id);
}
/**
 * \brief Starts playback on the mobile device.
 */
void Spotify::start_playback_on_mobile() {
    transfer_playback(mobile_device_id);
}
/**
 * \brief Switches playback between desktop and mobile devices.
 */
void Spotify::switch_player() {
    if (!refresh_tokens()) {
        return;
    }
    update_devices();
    string url = "https://api.spotify.com/v1/me/player";
    auto response = Get(
        Url {url},
        Header {{"Authorization", authorization_header},{"Content-Type", content_type}}
    );
    if (response.status_code == 204) {
        play_pause();
        return;
    }
    if (response.status_code != 200) {
        sp_logger.logg_and_print("Error: Status Code {} - {}", response.status_code, response.text);
        return;
    }
    auto playback_details = parse(response.text);
    string current_device_id = playback_details["device"]["id"];
    if (current_device_id == desktop_device_id) {
        start_playback_on_mobile();
    }
    else if (current_device_id == mobile_device_id) {
        start_playback_on_desktop();
    }
    else {
        sp_logger.logg_and_print("Device not added");
    }
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
        sp_logger.logg_and_print("no active device");
    }
    else if (response.status_code != 204) {
        sp_logger.logg_and_print("Error: Status Code {} - {}", response.status_code, response.text);
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
    string url = "https://api.spotify.com/v1/me/player/currently-playing";
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
        sp_logger.logg_and_print("tokens not refreshed in Spotify::play_pause()");
        return;
    }
    if (is_spotify_playing()) {
        sp_logger.logg_and_logg("is_spotify_playing() == true");
        pause_song();
        return;
    }
    if (play_song() == 204) {
        sp_logger.logg_and_logg("play_song() == 204");
        return;
    }
    if (is_spotify_open()) {
        sp_logger.logg_and_logg("is_spotify_open() == true");
        start_playback_on_desktop();
    }
    else {
        sp_logger.logg_and_logg("starting Spotify");
        thread t(start_playback_sp_thread);
        t.detach();
    }
}
/**
 * \brief Sends a POST request to skip to the next or previous song.
 * \param url URL for the POST request.
 */
void Spotify::post_next_or_prev(string url) {
    if (!refresh_tokens()) {
        return;
    }
    auto response = Post(
        Url {url},
        Header {{"Authorization", authorization_header},{"Content-Type", content_type}}
    );
    if (response.status_code != 204) {
        sp_logger.logg_and_print("post_next_or_prev() - Error: Status Code {} - {}", response.status_code, response.text);
    }
}
/**
 * \brief Skips to the next Spotify song.
 */
void Spotify::next_song() {
    string url = "https://api.spotify.com/v1/me/player/next";
    post_next_or_prev(url);
}
/**
 * \brief Skips to the previous Spotify song.
 */
void Spotify::prev_song() {
    string url = "https://api.spotify.com/v1/me/player/previous";
    post_next_or_prev(url);
}
