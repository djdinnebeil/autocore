/**
\file itunes_c.ixx
\brief Declarations for iTunes integration.
*/
export module itunes_c;
import visual;
import <Windows.h>;
import <comdef.h>;
import <atlbase.h>;

struct TrackInfo {
    wstring name;
    wstring artist;
    wstring album;
    int duration;
    wstring location;
};

export class iTunes {
public:
    iTunes();
    ~iTunes();
    void play_pause();
    void next_song();
    void prev_song();
    void stop_song();
    void initialize_com();
    void finalize_com();
    vector<wstring> song_history;
    wstring last_retrieved_song;
    wstring get_current_track();
    TrackInfo current_track;
    bool initialized;
    bool is_playing();
    wstring track_location;
    HRESULT hr;
    CComPtr<IDispatch> iTunes_app = nullptr;
    CComPtr<IDispatch> p_current_track = nullptr;
    CComPtr<IDispatch> get_current_track_com_object();
    TrackInfo get_track_info();
    int get_current_playback_position();
    int remaining_song_duration;
    bool end_thread;
    thread iTunes_thread;
    void start_iTunes_thread();
    void remove_track();
    void recycle_bin_track();
    void delete_track();
    int tab_end = 3;
    bool auto_start = false;
    void set_config();
};

export extern iTunes ac_iTunes;
export extern mutex iT_mtx;
export extern mutex history_mtx;
export extern condition_variable iT_cv;
export extern bool iT_playback_state_change;

export {
    void print_iTunes_songs();
    void print_next_up_song_list();
    void iTunes_play_pause();
    void iTunes_prev_song();
    void iTunes_stop_song();
    string replace_tabs_with_brackets(const string& input);
}
