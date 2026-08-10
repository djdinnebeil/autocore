module itunes_c;

import std;
import itunes_x;

iTunes ac_iTunes;
std::mutex iT_mtx;
std::mutex history_mtx;
std::condition_variable iT_cv;
bool iT_playback_state_change = false;

iTunes::iTunes() {
    set_config();
    if (auto_start) {
        initialize_com();
    }
}

iTunes::~iTunes() {
    finalize_com();
}
