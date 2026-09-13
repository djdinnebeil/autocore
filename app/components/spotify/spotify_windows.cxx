/**
\file spotify_windows.cxx
\brief Implements Spotify window detection and activation.
*/
module spotify_client;

import std;
import auto_core.core.keyboard;
import auto_core.taskbar;
import auto_core.core.thread;
import spotify_component;

import <json.hpp>;
import <cpr/cpr.h>;
import <chrono>;

using std::stoll;

using namespace cpr;

namespace this_thread = std::this_thread;
namespace chrono = std::chrono;

/**
 * \brief Callback function to enumerate Spotify windows.
 * \param hwnd Handle to the window.
 * \param lParam Application-defined parameter.
 * \return TRUE to continue enumeration, FALSE to stop.
 */
BOOL CALLBACK enum_spotify_premium_window(HWND hwnd, LPARAM lParam) {
    const size_t max_length = 15;
    const int length = GetWindowTextLength(hwnd);
    if (length != max_length) {
        return TRUE;
    }
    std::wstring window_title(length, L'\0');
    if (!GetWindowTextW(hwnd, &window_title[0], length + 1)) {
        return TRUE;
    }
    if (window_title == L"Spotify Premium") {
        *reinterpret_cast<bool*>(lParam) = true;
        spotify_component.logg_and_print("spotify window found");
        spotify_window_hwnd = hwnd;
        return FALSE;
    }
    return TRUE;
}
/**
 * \brief Activates the Spotify window.
 */
void Spotify::activate() {
    if (ac::taskbar::try_activate_native("spotify")) {
        return;
    }
    spotify_component.logg_and_print(
        "Spotify is not present in the native taskbar snapshot."
    );
}
/**
 * \brief Checks if the Spotify application is open.
 * \return True if Spotify is open, false otherwise.
 */
bool Spotify::is_spotify_open() {
    bool spotifyWindowFound = false;
    EnumWindows(enum_spotify_premium_window, reinterpret_cast<LPARAM>(&spotifyWindowFound));
    return spotifyWindowFound;
}
/**
 * \brief Starts the Spotify playback thread.
 * \todo Develop a more responsive display the current song when starting Spotify.
 */
void start_spotify_desktop_playback() {
    ac_spotify.activate();
    int total_sleep_time {};
    int sleep_duration = 500;
    int time_limit_ms = 10000;
    int processing_delay = 1500;
    while (!ac_spotify.is_spotify_open() && total_sleep_time < time_limit_ms) {
        this_thread::sleep_for(chrono::milliseconds(sleep_duration));
        total_sleep_time += sleep_duration;
    }
    this_thread::sleep_for(chrono::milliseconds(processing_delay));
    ac_spotify.start_playback_on_desktop();
    Sleep(1500);
    ac_spotify.get_current_song();
}
