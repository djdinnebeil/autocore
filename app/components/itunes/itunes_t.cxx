module itunes_t;

import std;
import clock;

import itunes_c;
import itunes_x;
import <Windows.h>;

namespace chrono = std::chrono;

void iTunes_next_song() {
    itunes_component.logg_and_logg("iTunes_next_song()");
    ac_iTunes.next_song();
    iT_playback_state_change = true;
    iT_cv.notify_one();
}

void iTunes::start_iTunes_thread() {
    Sleep(25);
    itunes_component.logg_and_logg("iTunes_thread started");
    const int sleep_timerate_secs_playing = 5;
    const int sleep_timerate_secs_pause = 5;
    const int extra_time_ms = 100;
    const int processing_delay_ms = 252;
    Sleep(350);
    try {
        std::unique_lock<std::mutex> lock(iT_mtx);
        int sleep_time_secs;
        while (true) {
            iT_playback_state_change = false;
            if (ac_iTunes.initialized) {
                ac_iTunes.get_current_track();
            }
            if (ac_iTunes.is_playing()) {
                if (ac_iTunes.remaining_song_duration > sleep_timerate_secs_playing) {
                    sleep_time_secs = sleep_timerate_secs_playing;
                }
                else if (ac_iTunes.remaining_song_duration < sleep_timerate_secs_playing) {
                    sleep_time_secs = ac_iTunes.remaining_song_duration;
                    Sleep(extra_time_ms);
                }
            }
            else {
                sleep_time_secs = sleep_timerate_secs_pause;
            }
            itunes_component.logg("iTunes sleep time {} seconds at {}", sleep_time_secs, ac::clock::get_timestamp_with_seconds());
            if (iT_cv.wait_for(lock, chrono::seconds(sleep_time_secs), [] {return iT_playback_state_change; })) {
                if (ac_iTunes.end_thread) {
                    break;
                }
                itunes_component.logg("iTunes_playback_state_change at {}", ac::clock::get_timestamp_with_seconds());
                Sleep(processing_delay_ms);
            }
        }
    }
    catch (const std::exception& e) {
        itunes_component.logg_and_print("iTunes_song_thread() has crashed: {}", e.what());
    }
    catch (...) {
        itunes_component.logg_and_print("iTunes_song_thread() has crashed due to an unknown exception");
    }
    itunes_component.logg("end of iTunes thread");
}
