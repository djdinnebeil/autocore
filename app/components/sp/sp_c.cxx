/**
\file sp_c.cxx
\brief Defines the Spotify class instance and component command wrappers.
*/
module sp_c;

import std;

import auto_core.clipboard;
import auto_core.encoding;
import auto_core.paths;
import auto_core.thread;
import sp_x;

import <json.hpp>;
import <cpr/cpr.h>;
import <chrono>;

using std::stoll;
using namespace cpr;

HWND spotify_window_hwnd;
Spotify ac_spotify;
/**
 * \brief Constructs the Spotify object and initializes necessary data.
 */
Spotify::Spotify() {
    timerate = 55;
    start_timestamp = 0;
    tokens_path = ac::paths::star_directory() / "sp_tokens.rc";
    devices_path = ac::paths::star_directory() / "sp_devices.rc";
    codes_path = ac::paths::star_directory() / "sp_codes.rc";
    content_type = "application/json";
    content_length = "Content-Length: 0";
}
bool get_user_queue_thread_started = false; /// \todo refactor the code

/**
 * \brief Retrieves the user's Spotify queue in a separate thread.
 */
void get_user_sp_queue_thread() {
    std::string user_queue = ac_spotify.get_user_queue();
    sp_component.logg_and_print(user_queue);
    sp_component.insert_text_replacing_clipboard(
        ac::encoding::to_utf16(user_queue) + L"\n\n"
    );
    get_user_queue_thread_started = false;
}

/**
* \brief Retrieves the user's Spotify queue.
* \runtime
*/
void get_user_sp_queue() {
    sp_component.logg_and_logg("get_user_sp_queue()");
    if (!get_user_queue_thread_started) {
        get_user_queue_thread_started = true;
        std::thread t([=]() {ac::thread::run_with_exception_handling(get_user_sp_queue_thread, sp_component); });
        t.detach();
    }
}

/**
* \brief Prints the Spotify songs to the console.
* \runtime
*/
void print_spotify_songs() {
    sp_component.logg_and_logg("print_spotify_songs()");
    std::ostringstream song_text;
    ac_spotify.get_current_song();
    if (!ac_spotify.song_history.empty()) {
        for (const auto& song : ac_spotify.song_history) {
            if (song != ac_spotify.last_song) {
                song_text << song << '\n';
            }
        }
        ac_spotify.song_history.clear();
    }
    song_text << ac_spotify.last_song << '\n';
    std::string song_text_str = song_text.str();

    sp_component.printnl(song_text_str);
    sp_component.insert_text_replacing_clipboard(
        ac::encoding::to_utf16(song_text_str) + L"\n"
    );
}
/**
* \brief Toggles Spotify play/pause in a separate thread.
*/
void spotify_play_pause_thread() {
    ac_spotify.play_pause();
}
/**
* \brief Toggles Spotify play/pause.
*
* \runtime
*/
void spotify_play_pause() {
    sp_component.logg_and_logg("spotify_play_pause()");
    std::thread t(spotify_play_pause_thread);
    t.detach();
}
/**
 * \brief Skips to the previous Spotify song.
 */
void spotify_prev_song() {
    ac_spotify.prev_song();
}

/**
* \brief Switches Spotify playback device.
* \runtime
*/
void sp_switch_player() {
    sp_component.logg_and_logg("sp_switch_player()");
    ac_spotify.switch_player();
}
/**
 * \brief Downloads the album cover of the current song.
 */
void download_album_cover() {
    ac_spotify.download_album_cover();
}
