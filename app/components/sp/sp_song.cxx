/**
\file sp_song.cxx
\brief Implements Spotify song, metadata, formatting, and queue handling.
*/
module sp_c;

import visual;
import thread;
import sp_x;
import <json.hpp>;
import <cpr/cpr.h>;
import <chrono>;

using std::stoll;
using namespace cpr;

/**
 * \brief Parses a JSON std::string.
 * \param s JSON std::string to parse.
 * \return Parsed JSON object.
 */
json parse(const std::string& s) {
    return json::parse(s);
}
/**
 * \brief Formats the artist name(s).
 * \param artists JSON array of artists.
 * \return Formatted artist name(s) as a std::string.
 */
std::string Spotify::format_artist_name(const json& artists) {
    std::string artist {};
    if (artists.size() == 1) {
        artist = artists[0]["name"];
    }
    else {
        for (size_t i = 0; i < artists.size(); ++i) {
            artist += artists[i]["name"];
            if (i < artists.size() - 1) {
                artist += ", ";
            }
        }
    }
    return artist;
}

SongMetadata Spotify::extract_song_metadata(const json& song_details) {
    SongMetadata meta;
    meta.name = song_details["name"];
    meta.artist = format_artist_name(song_details["artists"]);
    meta.album = song_details["album"]["name"];
    meta.duration_seconds = song_details["duration_ms"] / 1000;
    return meta;
}

/**
 * \brief Retrieves the current Spotify song.
 */
void Spotify::get_current_song() {
    sp_logger.logg("get_current_song()");
    if (!refresh_tokens()) {
        return;
    }
    std::string url = "https://api.spotify.com/v1/me/player/currently-playing";
    auto response = Get(
        Url {url},
        Header {{"Authorization", authorization_header},{"Content-Type", content_type}}
    );
    last_status_code = 1;
    if (response.status_code != 200) {
        if (response.status_code == 204) {
            sp_logger.logg("{} status code", response.status_code);
            is_playing = false;
        }
        else {
            sp_logger.logg_and_print("{} status code", response.status_code);
        }
        return;
    }
    auto song_details = parse(response.text);
    is_playing = song_details["is_playing"];
    if (!song_details["item"].contains("name")) {
        sp_logger.logg_and_logg("djai++ is talking");
        last_status_code = 15;
        return;
    }

    SongMetadata meta = extract_song_metadata(song_details["item"]);
    std::string current_song = format_song_title(meta);
    if (current_song == last_song) {
        return;
    }
    calculate_remaining_song_duration_ms(song_details);
    sp_logger.loggnl_and_loggnl("now playing: ");
    sp_logger.logg_and_print(current_song);
    last_song = current_song;
    song_history.push_back(current_song);
    track_spotify_history(meta);
    return;
}
/**
 * \brief Calculates the remaining song duration in milliseconds.
 * \param song_details JSON object containing song details.
 */
void Spotify::calculate_remaining_song_duration_ms(const json& song_details) {
    int progress_ms = song_details["progress_ms"];
    int duration_ms = song_details["item"]["duration_ms"];
    remaining_song_duration_ms = duration_ms - progress_ms;
}

/**
 * \brief Formats the song title.
 * \param song_details JSON object containing song details.
 * \return Formatted song title as a std::string.
 */
std::string Spotify::format_song_title(const SongMetadata& meta) {
    std::ostringstream output;
    std::ostringstream dur;
    dur << meta.duration_seconds / 60 << ":" << std::setw(2) << std::setfill('0') << meta.duration_seconds % 60;
    output << '[' << meta.name << "] [" << meta.artist << "] [" << meta.album << "] [" << dur.str() << ']';
    return output.str();
}

std::string Spotify::format_song_title_user_queue(const json& song_details) {
    const std::string name = song_details["name"];
    const std::string artist = format_artist_name(song_details["artists"]);
    const std::string album = song_details["album"]["name"];
    int duration = song_details["duration_ms"] / 1000;
    std::ostringstream output;
    std::ostringstream dur;
    dur << duration / 60 << ":" << std::setw(2) << std::setfill('0') << duration % 60;
    output << '[' << name << "] [" << artist << "] [" << album << "] [" << dur.str() << ']';
    return output.str();
}

/**
 * \brief Retrieves the user's Spotify queue.
 * \return Formatted user queue as a std::string.
 */
std::string Spotify::get_user_queue() {
    try {
        if (!refresh_tokens()) {
            return "Unable to refresh tokens";
        }
        std::string url = "https://api.spotify.com/v1/me/player/queue";
        auto response = Get(
            Url {url},
            Header {{"Authorization", authorization_header},{"Content-Type", content_type}}
        );
        if (response.status_code != 200) {
            sp_logger.logg_and_print("Failed to retrieve queue");
            return "";
        }
        json queue_details = parse(response.text);
        std::ostringstream output;
        std::string current_song;
        if (!queue_details["currently_playing"].is_null()) {
            current_song = format_song_title_user_queue(queue_details["currently_playing"]);
            song_history_contains(current_song);
        }
        for (const auto& item : queue_details["queue"]) {
            std::string song = format_song_title_user_queue(item);
            if (!song_history_contains(song)) {
                output << song << '\n';
            }
        }
        output << current_song;
        return output.str();
    }
    catch (...) {
        sp_logger.logg_and_print("An exception occurred in get_user_queue");
        return "";
    }
}
/**
 * \brief Downloads the current song's album cover.
 * \return True if the album cover was downloaded successfully, false otherwise.
 */
bool Spotify::download_album_cover() {
    if (!refresh_tokens()) {
        return false;
    }
    std::string url = "https://api.spotify.com/v1/me/player/currently-playing";
    auto response = Get(
        Url {url},
        Header {{"Authorization", authorization_header},{"Content-Type", content_type}}
    );
    if (response.status_code != 200) {
        sp_logger.logg_and_print("Failed to retrieve song");
        return "";
    }
    auto song_details = parse(response.text);
    std::string album_cover_url;
    if (!song_details["item"]["album"]["images"].empty()) {
        album_cover_url = song_details["item"]["album"]["images"][0]["url"].get<std::string>();
    }
    else {
        return false;
    }
    auto album_response = Get(Url {album_cover_url});
    std::string filepath = "cover.jpg";
    if (album_response.status_code == 200) {
        std::ofstream file(filepath, std::ios::binary);
        file.write(album_response.text.c_str(), album_response.text.size());
        file.close();
        return true;
    }
    else {
        return false;
    }
}
/**
 * \brief Checks if the song history contains the specified song.
 * \param current_song Song to check.
 * \return True if the song is in the history, false otherwise.
 */
bool Spotify::song_history_contains(std::string current_song) {
    for (const auto& song : song_history_array) {
        if (song == current_song) {
            return true;
        }
    }
    song_history_array[song_history_index] = current_song;
    song_history_index += 1;
    if (song_history_index == string_array_size) {
        song_history_index = 0;
    }
    return false;
}
