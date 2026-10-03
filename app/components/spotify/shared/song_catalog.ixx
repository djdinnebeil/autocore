/**
 * \file song_catalog.ixx
 * \brief Spotify song-format tokens and track-object rendering.
 *
 * Values come from the track object already returned by currently-playing
 * and queue. This module does not call the Spotify Web API.
 */
export module spotify_song_catalog;

import std;
import spotify_song_template;
import <json.hpp>;

export namespace spotify::catalog {

inline constexpr std::array<std::string_view, 11> tokens {
    "name",
    "artist",
    "album",
    "duration",
    "track_number",
    "disc_number",
    "release_date",
    "explicit",
    "id",
    "uri",
    "album_type"
};

[[nodiscard]] inline std::span<const std::string_view> known_tokens() noexcept {
    return tokens;
}

[[nodiscard]] std::string join_artists(const nlohmann::json& artists);

[[nodiscard]] std::string render_duration_ms(long long duration_ms);

[[nodiscard]] std::string render_token(
    std::string_view token,
    const nlohmann::json& track
);

[[nodiscard]] std::string format_track(
    const spotify::song::Compiled& compiled,
    const nlohmann::json& track
);

[[nodiscard]] std::string preview_value(std::string_view token);

[[nodiscard]] nlohmann::json preview_track();

[[nodiscard]] inline spotify::song::CompileResult compile_utf8(
    std::string_view bytes
) {
    return spotify::song::compile_utf8(bytes, known_tokens());
}

[[nodiscard]] inline std::optional<std::string> accepted_song_format(
    std::string_view text
) {
    return spotify::song::accepted_song_format(text, known_tokens());
}

[[nodiscard]] inline spotify::song::LoadedSongFormat load_song_format_file(
    const std::filesystem::path& path
) {
    return spotify::song::load_song_format_file(path, known_tokens());
}

[[nodiscard]] const spotify::song::Compiled& default_compiled();

} // namespace spotify::catalog

namespace {

[[nodiscard]] const nlohmann::json* object_field(
    const nlohmann::json& object,
    const char* key
) {
    if (!object.is_object() || !object.contains(key)) {
        return nullptr;
    }
    return &object.at(key);
}

[[nodiscard]] std::string json_string(
    const nlohmann::json& object,
    const char* key
) {
    const nlohmann::json* value = object_field(object, key);
    if (value == nullptr || !value->is_string()) {
        return {};
    }
    return value->get<std::string>();
}

[[nodiscard]] std::optional<long long> json_integer(
    const nlohmann::json& object,
    const char* key
) {
    const nlohmann::json* value = object_field(object, key);
    if (value == nullptr || value->is_null()) {
        return std::nullopt;
    }
    if (value->is_number_unsigned()) {
        const auto number = value->get<unsigned long long>();
        if (number > static_cast<unsigned long long>(LLONG_MAX)) {
            return std::nullopt;
        }
        return static_cast<long long>(number);
    }
    if (value->is_number_integer()) {
        return value->get<long long>();
    }
    return std::nullopt;
}

} // namespace

namespace spotify::catalog {

std::string join_artists(const nlohmann::json& artists) {
    if (!artists.is_array()) {
        return {};
    }

    if (artists.size() == 1) {
        if (!artists[0].is_object()) {
            return {};
        }
        return json_string(artists[0], "name");
    }

    std::string joined;
    for (std::size_t index = 0; index < artists.size(); ++index) {
        if (index > 0) {
            joined += ", ";
        }
        if (artists[index].is_object()) {
            joined += json_string(artists[index], "name");
        }
    }
    return joined;
}

std::string render_duration_ms(const long long duration_ms) {
    if (duration_ms < 0) {
        return {};
    }
    const long long seconds = duration_ms / 1000;
    const long long minutes = seconds / 60;
    const long long remainder = seconds % 60;
    std::string rendered = std::to_string(minutes);
    rendered.push_back(':');
    if (remainder < 10) {
        rendered.push_back('0');
    }
    rendered += std::to_string(remainder);
    return rendered;
}

std::string render_token(
    const std::string_view token,
    const nlohmann::json& track
) {
    if (!track.is_object()) {
        return {};
    }
    if (token == "name") {
        return json_string(track, "name");
    }
    if (token == "artist") {
        if (!track.contains("artists")) {
            return {};
        }
        return join_artists(track.at("artists"));
    }
    if (token == "album") {
        const nlohmann::json* album = object_field(track, "album");
        if (album == nullptr || !album->is_object()) {
            return {};
        }
        return json_string(*album, "name");
    }
    if (token == "duration") {
        const auto duration_ms = json_integer(track, "duration_ms");
        if (!duration_ms) {
            return {};
        }
        return render_duration_ms(*duration_ms);
    }
    if (token == "track_number") {
        const auto number = json_integer(track, "track_number");
        if (!number) {
            return {};
        }
        return std::to_string(*number);
    }
    if (token == "disc_number") {
        const auto number = json_integer(track, "disc_number");
        if (!number) {
            return {};
        }
        return std::to_string(*number);
    }
    if (token == "release_date") {
        const nlohmann::json* album = object_field(track, "album");
        if (album == nullptr || !album->is_object()) {
            return {};
        }
        return json_string(*album, "release_date");
    }
    if (token == "explicit") {
        const nlohmann::json* value = object_field(track, "explicit");
        if (value == nullptr || !value->is_boolean()) {
            return {};
        }
        return value->get<bool>() ? "true" : "false";
    }
    if (token == "id") {
        return json_string(track, "id");
    }
    if (token == "uri") {
        return json_string(track, "uri");
    }
    if (token == "album_type") {
        const nlohmann::json* album = object_field(track, "album");
        if (album == nullptr || !album->is_object()) {
            return {};
        }
        return json_string(*album, "album_type");
    }
    return {};
}

std::string format_track(
    const spotify::song::Compiled& compiled,
    const nlohmann::json& track
) {
    return spotify::song::apply(
        compiled,
        [&](const std::string_view token) {
            return render_token(token, track);
        }
    );
}

std::string preview_value(const std::string_view token) {
    if (token == "name") {
        return "Night Drive";
    }
    if (token == "artist") {
        return "Artist One, Artist Two";
    }
    if (token == "album") {
        return "Sample Album";
    }
    if (token == "duration") {
        return render_duration_ms(125000);
    }
    if (token == "track_number") {
        return "7";
    }
    if (token == "disc_number") {
        return "1";
    }
    if (token == "release_date") {
        return "2020-05-01";
    }
    if (token == "explicit") {
        return "true";
    }
    if (token == "id") {
        return "sampleTrackId";
    }
    if (token == "uri") {
        return "spotify:track:sampleTrackId";
    }
    if (token == "album_type") {
        return "album";
    }
    return {};
}

nlohmann::json preview_track() {
    return nlohmann::json::parse(R"({
        "name": "Night Drive",
        "artists": [
            {"name": "Artist One"},
            {"name": "Artist Two"}
        ],
        "album": {
            "name": "Sample Album",
            "release_date": "2020-05-01",
            "album_type": "album"
        },
        "duration_ms": 125000,
        "track_number": 7,
        "disc_number": 1,
        "explicit": true,
        "id": "sampleTrackId",
        "uri": "spotify:track:sampleTrackId"
    })");
}

const spotify::song::Compiled& default_compiled() {
    static const spotify::song::Compiled compiled = [] {
        const auto result = compile_utf8(spotify::song::default_template);
        return result.ok ? result.compiled : spotify::song::Compiled {};
    }();
    return compiled;
}

} // namespace spotify::catalog
