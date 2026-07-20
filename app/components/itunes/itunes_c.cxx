module itunes_c;
import visual;
import itunes_x;

iTunes ac_iTunes;
mutex iT_mtx;
mutex history_mtx;
condition_variable iT_cv;
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
