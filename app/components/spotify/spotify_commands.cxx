/**
\file spotify_commands.cxx
\brief Defines the Spotify component command actions.
*/
module spotify_client;

import std;

import auto_core.core.clipboard;
import auto_core.core.encoding;
import auto_core.core.paths;
import auto_core.core.thread;
import spotify_component;

import <json.hpp>;
import <cpr/cpr.h>;
import <chrono>;

using std::stoll;
using namespace cpr;

/**
 * \brief Retrieves and inserts the user's Spotify queue.
 */
void spotify_get_queue() {
    spotify_component.log_and_log("spotify_get_queue()");
    std::string user_queue = ac_spotify.get_user_queue();
    spotify_component.log_and_print(user_queue);
    spotify_component.insert_text_replacing_clipboard(
        ac::encoding::to_utf16(user_queue) + L"\n\n"
    );
}

/**
* \brief Prints the Spotify songs to the console.
* \runtime
*/
void spotify_print_songs() {
    spotify_component.log_and_log("spotify_print_songs()");
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

    spotify_component.printnl(song_text_str);
    spotify_component.insert_text_replacing_clipboard(
        ac::encoding::to_utf16(song_text_str) + L"\n"
    );
}
/**
* \brief Toggles Spotify play/pause.
*
* \runtime
*/
void spotify_play_pause() {
    spotify_component.log_and_log("spotify_play_pause()");
    ac_spotify.play_pause();
}
/**
 * \brief Skips to the previous Spotify song.
 */
void spotify_previous_song() {
    ac_spotify.prev_song();
}

/**
* \brief Switches Spotify playback device.
* \runtime
*/
void spotify_switch_player() {
    spotify_component.log_and_log("spotify_switch_player()");
    ac_spotify.switch_player();
}
/**
 * \brief Downloads the album cover of the current song.
 */
void spotify_download_album_cover() {
    ac_spotify.download_album_cover();
}
