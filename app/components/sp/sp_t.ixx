/**
\file sp_t.ixx
\brief Provides thread management and playback control for Spotify.
*/
export module sp_t;

import std;
import auto_core.clock;

import sp_c;
import sp_x;

namespace chrono = std::chrono;

export {
	void spotify_next_song();
	void start_sp_song_thread();
}

export std::mutex sp_mtx;
export std::condition_variable sp_cv;
export bool sp_playback_state_change;

const int sleep_timerate_ms = 15000;
const int speed_boost_ms = 5000;
const int speed_boost_code = 15;
const int extra_time = 1500;
const int processing_delay = 450;

/**
 * \brief Starts the Spotify song thread.
 */
void start_sp_song_thread() {
    std::thread t(sp_song_thread);
    t.detach();
}

/**
 * \brief Signals Spotify to play the next song and notifies the thread.
 * \runtime
 */
void spotify_next_song() {
    sp_component.logg_and_logg("spotify_next_song()");
    ac_spotify.next_song();
    sp_playback_state_change = true;
    sp_cv.notify_one();
}
/**
 * \brief The main function for the Spotify song thread.
 *
 * This function handles the timing for checking the current song and adjusts
 * the sleep time based on the song's remaining duration and playback state.
 */
void sp_song_thread() {
    sp_component.logg_and_logg("sp_thread started");
    Sleep(350);
    try {
        std::unique_lock<std::mutex> lock(sp_mtx);
        int sleep_time_ms;
        ac_spotify.get_current_song();
        if (ac_spotify.remaining_song_duration_ms == 0) {
            ac_spotify.remaining_song_duration_ms = sleep_timerate_ms;
        }
        while (true) {
            sp_playback_state_change = false;
            if (ac_spotify.reauthorization_required) {
                sp_component.loggnl("Spotify authorization required - ");
                sleep_time_ms = sleep_timerate_ms;
            }
            else if (!ac_spotify.is_playing) {
                sp_component.loggnl("Spotify not playing - ");
                sleep_time_ms = sleep_timerate_ms;
            }
            else if (ac_spotify.last_status_code == speed_boost_code) {
                sp_component.logg_and_print("speed boost!");
                sleep_time_ms = speed_boost_ms;
            }
            else if (ac_spotify.remaining_song_duration_ms < sleep_timerate_ms) {
                sleep_time_ms = ac_spotify.remaining_song_duration_ms + extra_time;
            }
            else {
                sleep_time_ms = sleep_timerate_ms;
            }
            sp_component.logg("sleep time {} seconds at {}", sleep_time_ms / 1000, ac::clock::get_timestamp_with_seconds());
            if (sp_cv.wait_for(lock, chrono::milliseconds(sleep_time_ms), [] {return sp_playback_state_change; })) {
                if (ac_spotify.end_thread) {
                    break;
                }
                sp_component.logg("sp_playback_state_change at {}", ac::clock::get_timestamp_with_seconds());
                Sleep(processing_delay);
            }
            ac_spotify.get_current_song();
        }
    }
    catch (const std::exception& e) {
        sp_component.logg_and_print("sp_song_thread() has crashed: {}", e.what());
    }
    catch (...) {
        sp_component.logg_and_print("sp_song_thread() has crashed due to an unknown exception");
    }
}