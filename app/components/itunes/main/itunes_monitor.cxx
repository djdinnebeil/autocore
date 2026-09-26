module itunes_monitor;

import std;
import auto_core.core.clock;

import itunes_client;
import itunes_component;
import itunes_runtime;
import <Windows.h>;

namespace chrono = std::chrono;

namespace {
    class component_playback_monitor final :
        public itunes::runtime::playback_monitor {
    public:
        void playback_state_changed() override {
            itunes_playback_state_change = true;
            itunes_condition.notify_one();
        }
    };

    component_playback_monitor playback_monitor;
}

void itunes_next_song() {
    itunes_component.log_main("itunes_next_song()");
    itunes::runtime::next_song(ac_itunes, playback_monitor);
}

void itunes_client::start_itunes_thread() {
    Sleep(25);
    itunes_component.log_main("itunes_thread started");
    const int sleep_timerate_secs_playing = 5;
    const int sleep_timerate_secs_pause = 5;
    const int extra_time_ms = 100;
    const int processing_delay_ms = 252;
    Sleep(350);
    try {
        std::unique_lock<std::mutex> lock(itunes_mutex);
        while (!ac_itunes.end_thread.load()) {
            itunes_playback_state_change = false;
            if (ac_itunes.is_initialized()) {
                static_cast<void>(ac_itunes.get_current_track());
            }
            if (ac_itunes.end_thread.load()) {
                break;
            }

            int sleep_time_secs = sleep_timerate_secs_pause;
            if (ac_itunes.is_playing()) {
                const int remaining = ac_itunes.remaining_song_duration.load();
                if (remaining > 0) {
                    sleep_time_secs = (std::min)(
                        remaining,
                        sleep_timerate_secs_playing
                    );
                }
                if (remaining > 0 && remaining <= sleep_timerate_secs_playing) {
                    Sleep(extra_time_ms);
                }
            }
            itunes_component.log("iTunes sleep time {} seconds at {}", sleep_time_secs, ac::clock::get_timestamp_with_seconds());
            if (itunes_condition.wait_for(lock, chrono::seconds(sleep_time_secs), [] {
                    return itunes_playback_state_change.load() ||
                           ac_itunes.end_thread.load();
                })) {
                if (ac_itunes.end_thread.load()) {
                    break;
                }
                itunes_component.log("itunes_playback_state_change at {}", ac::clock::get_timestamp_with_seconds());
                Sleep(processing_delay_ms);
            }
        }
    }
    catch (const std::exception& e) {
        itunes_component.log_print("itunes_song_thread() has crashed: {}", e.what());
    }
    catch (...) {
        itunes_component.log_print("itunes_song_thread() has crashed due to an unknown exception");
    }
    itunes_component.log("end of iTunes thread");
}
