module;

#include "../shared/listening_credit.hpp"

module itunes_monitor;

import std;
import auto_core.core.clock;
import auto_core.core.encoding;

import itunes_client;
import itunes_component;
import itunes_db_client;
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

std::string observed_at_now() {
    SYSTEMTIME time {};
    GetLocalTime(&time);
    return std::format(
        "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
        time.wYear,
        time.wMonth,
        time.wDay,
        time.wHour,
        time.wMinute,
        time.wSecond
    );
}

void report_listening_sample(
    const itunes::listening::WakeKind wake,
    const int scheduled_sleep_seconds,
    const int idle_interval_seconds,
    const itunes_client::ListeningSample& sample
) {
    if (!sample.has_track) {
        return;
    }
    itunes::db::Observation observation;
    observation.track_id = sample.track_id;
    observation.title = ac::encoding::to_utf8(sample.title);
    observation.artist = ac::encoding::to_utf8(sample.artist);
    observation.album = ac::encoding::to_utf8(sample.album);
    observation.duration_seconds = sample.duration_seconds;
    observation.credit_seconds = itunes::listening::credit_seconds({
        .wake = wake,
        .scheduled_sleep_seconds = scheduled_sleep_seconds,
        .playing = sample.playing,
        .position_seconds = sample.position_seconds,
        .idle_interval_seconds = idle_interval_seconds,
    });
    observation.observed_at = observed_at_now();
    if (const auto recorded = itunes::db::observe(observation); !recorded) {
        itunes_component.log("{}", recorded.error());
    }
}

void itunes_client::start_itunes_thread() {
    Sleep(25);
    itunes_component.log_main("itunes_thread started");
    const int sleep_timerate_secs_playing = 5;
    const int sleep_timerate_secs_pause = 5;
    const int extra_time_ms = 100;
    const int processing_delay_ms = 252;
    Sleep(350);
    itunes::listening::WakeKind pending_wake = itunes::listening::WakeKind::initial;
    int pending_sleep_seconds = 0;
    try {
        std::unique_lock<std::mutex> lock(itunes_mutex);
        while (!ac_itunes.end_thread.load()) {
            itunes_playback_state_change = false;
            if (ac_itunes.is_initialized()) {
                report_listening_sample(
                    pending_wake,
                    pending_sleep_seconds,
                    sleep_timerate_secs_pause,
                    ac_itunes.capture_listening_sample()
                );
            }
            if (ac_itunes.end_thread.load()) {
                break;
            }

            int sleep_time_secs = sleep_timerate_secs_pause;
            const bool playing_for_sleep = ac_itunes.is_playing();
            if (playing_for_sleep) {
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
                pending_wake = itunes::listening::WakeKind::special;
                pending_sleep_seconds = 0;
            }
            else {
                pending_wake = playing_for_sleep
                    ? itunes::listening::WakeKind::playing_timeout
                    : itunes::listening::WakeKind::idle_timeout;
                pending_sleep_seconds = sleep_time_secs;
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
