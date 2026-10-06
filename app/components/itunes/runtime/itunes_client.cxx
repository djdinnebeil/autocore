module;

#include "../shared/song_template_detail.hpp"

module itunes_client;

import std;
import itunes_component;

itunes_client ac_itunes;
std::mutex itunes_mutex;
std::mutex history_mtx;
std::condition_variable itunes_condition;
std::atomic_bool itunes_playback_state_change {false};

itunes_client::itunes_client()
    : song_format {itunes::song::detail::default_compiled()} {}

itunes_client::~itunes_client() {
    shutdown();
}
