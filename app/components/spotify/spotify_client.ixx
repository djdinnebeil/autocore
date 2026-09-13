/**
\file spotify_client.ixx
\brief Provides Spotify component support for the Auto Core application.

This module defines the Spotify class with methods to control Spotify playback,
retrieve song information, and manage authentication tokens. It also includes
functions for handling Spotify-related tasks such as downloading album covers and
switching playback devices.

 */
export module spotify_client;

import std;
import auto_core.core.thread;
import spotify_component;
import <json.hpp>;
import <cpr/cpr.h>;
import <chrono>;

using json = nlohmann::json;
export extern HWND spotify_window_hwnd;

/** Keymap-facing Spotify playback helpers implemented in this component. */
export {
    void spotify_get_queue();
    void spotify_print_songs();
    void spotify_play_pause();
    void spotify_previous_song();
    void spotify_switch_player();
    void spotify_download_album_cover();
}

struct SongMetadata {
    std::string name;
    std::string artist;
    std::string album;
    int duration_seconds;
};

std::string get_datetime_stamp_local();
void track_spotify_history_update_or_insert(const SongMetadata& meta);
void track_spotify_history(const SongMetadata& meta);
std::string ascii_lower(std::string text);
bool device_name_equals_ignore_case(
    std::string_view left,
    std::string_view right
);

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
    std::string get_user_queue();
    int last_status_code;
    std::vector<std::string> song_history;
    std::string last_song;
    std::string client_id;
    std::string access_token;
    std::string refresh_token;
    std::string album_url;
    int timerate;
    bool check_timerate();
    void extract_tokens();
    bool refresh_tokens();
    int pause_song();
    int play_song();
    int music_song_count;
    int remaining_song_duration_ms;
    bool next_song_clicked;
    bool is_spotify_playing();
    std::string get_device_code(std::string_view name) const;
    void switch_player();
    bool song_history_contains(std::string song);
    int song_history_index = 0;
    int string_array_size = 52;
    std::array<std::string, 52> song_history_array;
    void load_config();
    std::string authorization_header;
    std::string content_type;
    std::string content_length;
    bool is_playing;
    void transfer_playback(std::string device_id);
    void post_next_or_prev(std::string url);
    bool is_spotify_open();
    void start_playback_on_desktop();
    std::string format_song_title_user_queue(const json& song_details);
    SongMetadata extract_song_metadata(const json& song_details);
    std::string format_song_title(const SongMetadata& meta);
    std::string format_artist_name(const json& artists);
    void calculate_remaining_song_duration_ms(const json& song_details);
    void update_devices();
    void activate();
    void save_config();
    void save_tokens();
    std::string first_configured_device_id() const;
    bool end_thread = false;

    time_t start_timestamp {};
    time_t refresh_token_expiration {};

    std::mutex refresh_token_mutex;
    bool reauthorization_required = false;
    bool reauthorization_warning_logged  = false;

    bool tokens_extracted = false;
    void check_refresh_token_expiration();

    std::filesystem::path tokens_path;
    std::filesystem::path config_path;
    std::vector<std::pair<std::string, std::string>> devices;
};

export extern Spotify ac_spotify;
