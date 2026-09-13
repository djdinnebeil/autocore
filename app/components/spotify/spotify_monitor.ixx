/**
\file spotify_monitor.ixx
\brief Provides thread management and playback control for Spotify.
*/
export module spotify_monitor;

import std;
import auto_core.core.clock;

import spotify_client;
import spotify_component;

namespace chrono = std::chrono;

export {
	/** Advances playback and notifies the monitor thread. */
	void spotify_next_song();
	void start_spotify_monitor();
	void stop_spotify_monitor();
}

std::mutex spotify_mtx;
std::condition_variable spotify_cv;
bool spotify_playback_state_change;

std::thread spotify_song_worker;

void spotify_monitor_loop();

const int sleep_timerate_ms = 15000;
const int speed_boost_ms = 5000;
const int speed_boost_code = 15;
const int extra_time = 1500;
const int processing_delay = 450;

/**
 * \brief Starts the Spotify song thread.
 */
void start_spotify_monitor() {
    if (!spotify_song_worker.joinable()) {
        ac_spotify.end_thread = false;
        spotify_song_worker = std::thread(spotify_monitor_loop);
    }
}

/**
 * \brief Stops and joins the Spotify song thread.
 */
void stop_spotify_monitor() {
    {
        const std::scoped_lock lock {spotify_mtx};
        ac_spotify.end_thread = true;
        spotify_playback_state_change = true;
    }
    spotify_cv.notify_one();

    if (spotify_song_worker.joinable()) {
        spotify_song_worker.join();
    }
}

/**
 * \brief Signals Spotify to play the next song and notifies the thread.
 * \runtime
 */
void spotify_next_song() {
    spotify_component.logg_and_logg("spotify_next_song()");
    ac_spotify.next_song();
    {
        const std::scoped_lock lock {spotify_mtx};
        spotify_playback_state_change = true;
    }
    spotify_cv.notify_one();
}
/**
 * \brief The main function for the Spotify song thread.
 *
 * This function handles the timing for checking the current song and adjusts
 * the sleep time based on the song's remaining duration and playback state.
 */
void spotify_monitor_loop() {
    spotify_component.logg_and_logg("Spotify monitor started");
    Sleep(350);
    try {
        std::unique_lock<std::mutex> lock(spotify_mtx);
        int sleep_time_ms;
        ac_spotify.get_current_song();
        if (ac_spotify.remaining_song_duration_ms == 0) {
            ac_spotify.remaining_song_duration_ms = sleep_timerate_ms;
        }
        while (true) {
            spotify_playback_state_change = false;
            if (ac_spotify.reauthorization_required) {
                spotify_component.loggnl("Spotify authorization required - ");
                sleep_time_ms = sleep_timerate_ms;
            }
            else if (!ac_spotify.is_playing) {
                spotify_component.loggnl("Spotify not playing - ");
                sleep_time_ms = sleep_timerate_ms;
            }
            else if (ac_spotify.last_status_code == speed_boost_code) {
                spotify_component.logg_and_print("speed boost!");
                sleep_time_ms = speed_boost_ms;
            }
            else if (ac_spotify.remaining_song_duration_ms < sleep_timerate_ms) {
                sleep_time_ms = ac_spotify.remaining_song_duration_ms + extra_time;
            }
            else {
                sleep_time_ms = sleep_timerate_ms;
            }
            spotify_component.logg("sleep time {} seconds at {}", sleep_time_ms / 1000, ac::clock::get_timestamp_with_seconds());
            if (spotify_cv.wait_for(lock, chrono::milliseconds(sleep_time_ms), [] {return spotify_playback_state_change; })) {
                if (ac_spotify.end_thread) {
                    break;
                }
                spotify_component.logg("spotify_playback_state_change at {}", ac::clock::get_timestamp_with_seconds());
                Sleep(processing_delay);
            }
            ac_spotify.get_current_song();
        }
    }
    catch (const std::exception& e) {
        spotify_component.logg_and_print("Spotify monitor crashed: {}", e.what());
    }
    catch (...) {
        spotify_component.logg_and_print("Spotify monitor crashed due to an unknown exception");
    }
}
