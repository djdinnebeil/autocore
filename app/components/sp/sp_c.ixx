/**
\file sp_c.ixx
\brief Provides Spotify component support for the Auto Core application.

This module defines the Spotify class with methods to control Spotify playback,
retrieve song information, and manage authentication tokens. It also includes
functions for handling Spotify-related tasks such as downloading album covers and
switching playback devices.

 */
export module sp_c;
import visual;
import thread;
import sp_x;
import <json.hpp>;
import <cpr/cpr.h>;
import <chrono>;

using json = nlohmann::json;
export extern HWND spotify_window_hwnd;

export {
    void get_user_sp_queue();
    void sp_song_thread();
    void start_sp_song_thread();
    void print_spotify_songs();
    void spotify_play_pause();
    void spotify_prev_song();
    void sp_switch_player();
    void download_album_cover();
}

struct SongMetadata {
    string name;
    string artist;
    string album;
    int duration_seconds;
};

string get_datetime_stamp_local();
void track_spotify_history_update_or_insert(const SongMetadata& meta);
void track_spotify_history(const SongMetadata& meta);

/**
 * \brief Spotify class to manage Spotify integration.
 *
 * This class handles Spotify playback control, token management, song retrieval, and
 * other related functionalities.
 */
class Spotify {
public:
    Spotify();
    void get_current_song();
    bool download_album_cover();
    void play_pause();
    void next_song();
    void prev_song();
    string get_user_queue();
    int last_status_code;
    vector<string> song_history;
    string last_song;
    string client_id;
    string client_secret;
    string credentials_64;
    string access_token;
    string refresh_token;
    string album_url;
    int timerate;
    bool check_timerate();
    void extract_tokens();
    bool refresh_tokens();
    int pause_song();
    int play_song();
    int music_song_count;
    string tokens_path;
    string devices_path;
    string codes_path;
    int remaining_song_duration_ms;
    bool next_song_clicked;
    bool is_spotify_playing();
    string desktop_device_id;
    string mobile_device_id;
    void switch_player();
    bool song_history_contains(string song);
    int song_history_index = 0;
    int string_array_size = 52;
    array<string, 52> song_history_array;
    void get_devices();
    void get_credentials();
    string authorization_header;
    string content_type;
    string content_length;
    bool is_playing;
    void transfer_playback(string device_id);
    void post_next_or_prev(string url);
    bool is_spotify_open();
    void start_playback_on_desktop();
    void start_playback_on_mobile();
    string format_song_title_user_queue(const json& song_details);
    SongMetadata extract_song_metadata(const json& song_details);
    string format_song_title(const SongMetadata& meta);
    string format_artist_name(const json& artists);
    void calculate_remaining_song_duration_ms(const json& song_details);
    void update_devices();
    int sp_position;
    void get_sp_position();
    void activate();
    bool end_thread;

    time_t start_timestamp {};
    time_t refresh_token_expiration {};

    mutex refresh_token_mutex;
    bool reauthorization_required = false;
    bool reauthorization_warning_logged  = false;

    bool tokens_extracted = false;
    void check_refresh_token_expiration();
};

export extern Spotify ac_spotify;
