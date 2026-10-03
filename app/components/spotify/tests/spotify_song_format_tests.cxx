// Spotify song.format template and catalog tests. No network.
#include "catch_amalgamated.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

import spotify_defaults;
import spotify_song_catalog;
import spotify_song_template;
import <json.hpp>;

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
}

nlohmann::json artist_named(std::string name) {
    nlohmann::json artist = nlohmann::json::object();
    artist["name"] = std::move(name);
    return artist;
}

nlohmann::json one_artist(std::string name) {
    nlohmann::json artists = nlohmann::json::array();
    artists.push_back(artist_named(std::move(name)));
    return artists;
}

nlohmann::json track_with(
    std::string name,
    nlohmann::json artists,
    std::string album,
    long long duration_ms
) {
    nlohmann::json album_object = nlohmann::json::object();
    album_object["name"] = std::move(album);
    nlohmann::json track = nlohmann::json::object();
    track["name"] = std::move(name);
    track["artists"] = std::move(artists);
    track["album"] = std::move(album_object);
    track["duration_ms"] = duration_ms;
    return track;
}

const spotify::song::Compiled& default_format() {
    return spotify::catalog::default_compiled();
}

std::filesystem::path test_directory(std::string_view name) {
    const auto directory = std::filesystem::temp_directory_path() /
        "ac_spotify_song_format" / name;
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    return directory;
}

} // namespace

TEST_CASE("Spotify duration rendering keeps local m:ss", "[spotify][format][unit]") {
    CHECK(spotify::catalog::render_duration_ms(0) == "0:00");
    CHECK(spotify::catalog::render_duration_ms(1500) == "0:01");
    CHECK(spotify::catalog::render_duration_ms(5000) == "0:05");
    CHECK(spotify::catalog::render_duration_ms(61000) == "1:01");
    CHECK(spotify::catalog::render_duration_ms(5400000) == "90:00");
    CHECK(spotify::catalog::render_duration_ms(-1).empty());
}

TEST_CASE("Default Spotify song format matches the current track line", "[spotify][format][unit]") {
    const auto compiled = spotify::catalog::compile_utf8(spotify::song::default_template);
    REQUIRE(compiled.ok);
    const auto track = track_with(
        "Song",
        one_artist("Artist"),
        "Album",
        0
    );
    CHECK(
        spotify::catalog::format_track(compiled.compiled, track) ==
        "[Song] [0:00] [Artist] [Album]"
    );
    CHECK(compiled.compiled.references("name"));
    CHECK(compiled.compiled.references("artist"));
    CHECK(compiled.compiled.references("album"));
    CHECK(compiled.compiled.references("duration"));
    CHECK_FALSE(compiled.compiled.references("id"));
}

TEST_CASE("Spotify artists stay joined with a comma and space", "[spotify][format][unit]") {
    CHECK(spotify::catalog::join_artists(one_artist("Artist")) == "Artist");
    nlohmann::json several_artists = nlohmann::json::array();
    several_artists.push_back(artist_named("Artist One"));
    several_artists.push_back(artist_named("Artist Two"));
    several_artists.push_back(artist_named("Artist Three"));
    CHECK(
        spotify::catalog::join_artists(several_artists) ==
        "Artist One, Artist Two, Artist Three"
    );
    CHECK(spotify::catalog::join_artists(nlohmann::json::array()).empty());

    nlohmann::json two_artists = nlohmann::json::array();
    two_artists.push_back(artist_named("Artist One"));
    two_artists.push_back(artist_named("Artist Two"));
    const auto several = track_with("Song", std::move(two_artists), "Album", 125000);
    CHECK(
        spotify::catalog::format_track(default_format(), several) ==
        "[Song] [2:05] [Artist One, Artist Two] [Album]"
    );
}

TEST_CASE("Spotify optional format fields render empty when absent", "[spotify][format][unit]") {
    const auto compiled = spotify::catalog::compile_utf8(
        "{track_number}|{disc_number}|{release_date}|{explicit}|{id}|{uri}|{album_type}"
    );
    REQUIRE(compiled.ok);
    const auto track = track_with("Song", one_artist("Artist"), "Album", 1000);
    CHECK(
        spotify::catalog::format_track(compiled.compiled, track) ==
        "||||||"
    );
}

TEST_CASE("Spotify additional format fields render from the track object", "[spotify][format][unit]") {
    const auto compiled = spotify::catalog::compile_utf8(
        "{track_number}|{disc_number}|{release_date}|{explicit}|{id}|{uri}|{album_type}"
    );
    REQUIRE(compiled.ok);

    auto track = spotify::catalog::preview_track();
    track["explicit"] = false;
    track["track_number"] = 0;
    CHECK(
        spotify::catalog::format_track(compiled.compiled, track) ==
        "0|1|2020-05-01|false|sampleTrackId|spotify:track:sampleTrackId|album"
    );

    track["explicit"] = true;
    CHECK(spotify::catalog::render_token("explicit", track) == "true");
    track.erase("explicit");
    CHECK(spotify::catalog::render_token("explicit", track).empty());
}

TEST_CASE("Spotify preview values match the sample track", "[spotify][format][unit]") {
    const auto compiled = spotify::catalog::compile_utf8(
        "[{name}] [{artist}] [{album}] [{duration}] {track_number} {explicit}"
    );
    REQUIRE(compiled.ok);
    const std::string from_json = spotify::catalog::format_track(
        compiled.compiled,
        spotify::catalog::preview_track()
    );
    const std::string from_preview = spotify::song::apply(
        compiled.compiled,
        spotify::catalog::preview_value
    );
    CHECK(from_json == from_preview);
    CHECK(from_json == "[Night Drive] [Artist One, Artist Two] [Sample Album] [2:05] 7 true");
}

TEST_CASE("Current-track and queue items share one Spotify song format", "[spotify][format][unit]") {
    const auto compiled = spotify::catalog::compile_utf8(
        "[{name}] [{artist}] [{album}] [{duration}] {id}"
    );
    REQUIRE(compiled.ok);

    const auto current_item = spotify::catalog::preview_track();
    const auto queue_item = spotify::catalog::preview_track();
    CHECK(
        spotify::catalog::format_track(compiled.compiled, current_item) ==
        spotify::catalog::format_track(compiled.compiled, queue_item)
    );
}

TEST_CASE("Spotify song format rejects invalid templates", "[spotify][format][unit]") {
    CHECK_FALSE(spotify::catalog::compile_utf8("").ok);
    CHECK_FALSE(spotify::catalog::compile_utf8("   ").ok);
    CHECK_FALSE(spotify::catalog::compile_utf8(" \t\n").ok);
    CHECK_FALSE(spotify::catalog::compile_utf8("{name").ok);
    CHECK_FALSE(spotify::catalog::compile_utf8("{}").ok);
    CHECK_FALSE(spotify::catalog::compile_utf8("{ name }").ok);
    CHECK_FALSE(spotify::catalog::compile_utf8("{Name}").ok);
    CHECK_FALSE(spotify::catalog::compile_utf8("{popularity}").ok);
    CHECK_FALSE(spotify::catalog::compile_utf8("line\nstill").ok);
    CHECK_FALSE(spotify::catalog::compile_utf8("[{name}]\n[{artist}]").ok);
    CHECK(spotify::catalog::compile_utf8("{popularity}").error.find("Unknown") != std::string::npos);

    const auto invalid = spotify::catalog::compile_utf8("{popularity}");
    CHECK_FALSE(invalid.ok);
    CHECK(
        spotify::catalog::format_track(invalid.compiled, spotify::catalog::preview_track()) ==
        spotify::catalog::format_track(default_format(), spotify::catalog::preview_track())
    );
}

TEST_CASE("Spotify song format strips one trailing newline", "[spotify][format][unit]") {
    CHECK(spotify::catalog::accepted_song_format("[{name}]\n") == "[{name}]");
    CHECK(spotify::catalog::accepted_song_format("[{name}]\r\n") == "[{name}]");
    CHECK(spotify::catalog::accepted_song_format("[{name}]\r") == "[{name}]");
    CHECK_FALSE(spotify::catalog::accepted_song_format("[{name}]\n\n").has_value());
    CHECK_FALSE(spotify::catalog::accepted_song_format("[{name}]\r\n\n").has_value());

    const auto compiled = spotify::catalog::compile_utf8("[{name}]\n");
    REQUIRE(compiled.ok);
    CHECK(compiled.compiled.references("name"));
    CHECK_FALSE(compiled.compiled.references("artist"));
}

TEST_CASE("Invalid Spotify song format does not produce replacement bytes", "[spotify][format][unit]") {
    CHECK_FALSE(spotify::catalog::accepted_song_format("{popularity}").has_value());
    CHECK_FALSE(spotify::catalog::accepted_song_format(std::string(4097, 'a')).has_value());
    CHECK(spotify::catalog::accepted_song_format(std::string(4096, 'a')) == std::string(4096, 'a'));
    CHECK_FALSE(spotify::catalog::accepted_song_format("\xff").has_value());
    CHECK_FALSE(spotify::catalog::compile_utf8("\xff").ok);
}

TEST_CASE("Missing Spotify song.format uses the compiled default", "[spotify][format][unit]") {
    const auto path = test_directory("missing") / "song.format";
    const auto loaded = spotify::catalog::load_song_format_file(path);

    CHECK_FALSE(loaded.invalid);
    CHECK(loaded.error.empty());
    CHECK(
        spotify::catalog::format_track(loaded.compiled, spotify::catalog::preview_track()) ==
        "[Night Drive] [2:05] [Artist One, Artist Two] [Sample Album]"
    );
    CHECK_FALSE(std::filesystem::exists(path));
}

TEST_CASE("Malformed or oversized Spotify song.format is not replaced", "[spotify][format][unit]") {
    const auto malformed_path = test_directory("malformed") / "song.format";
    std::filesystem::create_directories(malformed_path.parent_path());
    {
        std::ofstream output(malformed_path, std::ios::binary);
        output << "{popularity}";
    }

    const auto malformed = spotify::catalog::load_song_format_file(malformed_path);
    CHECK(malformed.invalid);
    CHECK(malformed.error.find("Unknown") != std::string::npos);
    CHECK(read_file(malformed_path) == "{popularity}");
    CHECK(
        spotify::catalog::format_track(malformed.compiled, spotify::catalog::preview_track()) ==
        "[Night Drive] [2:05] [Artist One, Artist Two] [Sample Album]"
    );

    const auto oversized_path = test_directory("oversized") / "song.format";
    std::filesystem::create_directories(oversized_path.parent_path());
    const std::string oversized_bytes(4097, 'a');
    {
        std::ofstream output(oversized_path, std::ios::binary);
        output << oversized_bytes;
    }

    const auto oversized = spotify::catalog::load_song_format_file(oversized_path);
    CHECK(oversized.invalid);
    CHECK(oversized.error == "Song format is larger than 4096 bytes.");
    CHECK(read_file(oversized_path) == oversized_bytes);
}

TEST_CASE("Spotify config directory defaults stay compiled", "[spotify][format][unit]") {
    CHECK(std::string {spotify::defaults::directory} == "components\\spotify");
    CHECK(
        std::string {spotify::defaults::ini_text} ==
        "[spotify]\n"
        "directory = components\\spotify\n"
        "auto_launch_oauth = off\n"
        "logging = on\n"
    );
    CHECK(spotify::defaults::ini_for("") == spotify::defaults::ini_text);
}

TEST_CASE("Accepted Spotify song format replaces through a new file", "[spotify][format][unit]") {
    const auto path = test_directory("replace") / "song.format";
    REQUIRE(spotify::song::replace_song_format(path, "[{name}]"));

    CHECK(read_file(path) == "[{name}]");
    CHECK_FALSE(std::filesystem::exists(
        std::filesystem::path {path.wstring() + L".new"}
    ));
    CHECK(path.filename() == "song.format");
}

TEST_CASE("Rejected Spotify song format leaves the existing file unchanged", "[spotify][format][unit]") {
    const auto path = test_directory("cancel") / "song.format";
    std::filesystem::create_directories(path.parent_path());
    {
        std::ofstream output(path, std::ios::binary);
        output << "[{name}]";
    }

    const auto rejected = spotify::catalog::accepted_song_format("{popularity}");
    CHECK_FALSE(rejected.has_value());
    if (rejected) {
        REQUIRE(spotify::song::replace_song_format(path, *rejected));
    }

    CHECK(read_file(path) == "[{name}]");
    CHECK_FALSE(std::filesystem::exists(
        std::filesystem::path {path.wstring() + L".new"}
    ));
}
