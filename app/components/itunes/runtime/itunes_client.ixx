/**
 * \file itunes_client.ixx
 * \brief COM iTunes client. All COM create/access/release run on one owner thread.
 *
 * See docs/itunes.md for privilege limits and reconnection. `ac_itunes` is the
 * process-wide client; mutexes and `itunes_condition` coordinate the monitor.
 */
module;

#include "../shared/library_format_detail.hpp"
#include "../shared/song_template_detail.hpp"

export module itunes_client;

import std;
import itunes_runtime;
import <Windows.h>;
import <comdef.h>;
import <atlbase.h>;

struct TrackInfo {
    std::wstring name;
    std::wstring artist;
    std::wstring album;
    int duration {};
    std::wstring location;
    bool has_database_id = false;
    long database_id = 0;
};

export class itunes_client final :
    public itunes::runtime::automation,
    public itunes::runtime::lifecycle {
public:
    itunes_client();
    ~itunes_client();
    void play_pause() override;
    void next_song() override;
    void prev_song() override;
    void stop_song() override;
    [[nodiscard]] bool initialize_com();
    [[nodiscard]] bool initialize_attempt() override;
    [[nodiscard]] bool is_initialized() const noexcept override;
    void shutdown() noexcept override;
    std::vector<std::wstring> song_history;
    std::wstring last_retrieved_song;
    std::wstring get_current_track() override;
    [[nodiscard]] bool has_current_track() const noexcept override;
    [[nodiscard]] std::filesystem::path remove_current_track() override;
    itunes::library_format::detail::LibraryFormat library_format {};
    bool auto_start = true;
    std::filesystem::path data_directory;
    itunes::song::detail::Compiled song_format;
    void set_config();

    struct ListeningSample {
        bool has_track = false;
        std::int64_t track_id = 0;
        std::wstring title;
        std::wstring artist;
        std::wstring album;
        int duration_seconds = 0;
        int position_seconds = 0;
        bool playing = false;
    };

    [[nodiscard]] ListeningSample capture_listening_sample();

private:
    [[nodiscard]] bool initialize_on_com_thread();
    void release_on_com_thread() noexcept;
    void invoke_player_method(const wchar_t* method_name);
    void invoke_player_method_on_com_thread(const wchar_t* method_name);
    [[nodiscard]] bool is_playing();
    [[nodiscard]] bool is_playing_on_com_thread();
    [[nodiscard]] int get_current_playback_position_on_com_thread();
    [[nodiscard]] CComPtr<IDispatch> get_current_track_com_object_on_com_thread();
    [[nodiscard]] TrackInfo get_track_info_on_com_thread();
    [[nodiscard]] std::wstring get_current_track_on_com_thread(ListeningSample* sample);
    void start_itunes_thread();

    itunes::runtime::serial_executor com_executor;
    std::mutex initialization_mutex;
    std::atomic_bool initialized {false};
    std::atomic_bool shutdown_requested {false};
    bool com_apartment_initialized = false;
    CComPtr<IDispatch> itunes_app = nullptr;
    CComPtr<IDispatch> p_current_track = nullptr;
    std::wstring track_location;
    std::atomic_int remaining_song_duration {-1};
    std::atomic_bool end_thread {false};
    std::thread itunes_thread;
};

/** Process-wide COM client. Use only through the owner thread / executor. */
export extern itunes_client ac_itunes;
export extern std::mutex itunes_mutex;
export extern std::mutex history_mtx;
export extern std::condition_variable itunes_condition;
export extern std::atomic_bool itunes_playback_state_change;

export {
    void print_itunes_songs();
    void print_next_up_song_list();
    void itunes_play_pause();
    void itunes_prev_song();
    void itunes_stop_song();
    std::string replace_tabs_with_brackets(const std::string& input);
}

export {
    void print_itunes_songs();
    void print_next_up_song_list();
    void itunes_play_pause();
    void itunes_prev_song();
    void itunes_stop_song();
    std::string replace_tabs_with_brackets(const std::string& input);
}
