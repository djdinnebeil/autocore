#include "catch_amalgamated.hpp"
#include "../shared/itunes_metadata_detail.hpp"
#include "../shared/song_template_detail.hpp"

#include <iterator>

namespace song = itunes::song::detail;
namespace metadata = itunes::metadata::detail;

namespace {

std::wstring render(const song::Compiled& compiled) {
    return song::apply(compiled, [](const std::string_view token) -> std::wstring {
        if (token == "name") {
            return L"Song";
        }
        if (token == "artist") {
            return L"Artist";
        }
        if (token == "album") {
            return L"Album";
        }
        if (token == "duration") {
            return L"3:42";
        }
        return L"unexpected";
    });
}

} // namespace

TEST_CASE("iTunes duration rendering keeps local m:ss", "[itunes][format][unit]") {
    CHECK(metadata::render_duration(245) == L"4:05");
    CHECK(metadata::render_duration(5) == L"0:05");
    CHECK(metadata::render_duration(0) == L"0:00");
}

TEST_CASE("Default iTunes song format matches the verified track line", "[itunes][format][unit]") {
    const auto compiled = song::compile_utf8(song::default_template);
    REQUIRE(compiled.ok);
    CHECK(render(compiled.compiled) == L"[Song] [Artist] [Album] [3:42]");
    CHECK(compiled.compiled.references("name"));
    CHECK(compiled.compiled.references("artist"));
    CHECK(compiled.compiled.references("album"));
    CHECK(compiled.compiled.references("duration"));
}

TEST_CASE("iTunes song format keeps literals and repeated tokens", "[itunes][format][unit]") {
    const auto compiled = song::compile_utf8("hello {name}-{name}");
    REQUIRE(compiled.ok);
    CHECK(render(compiled.compiled) == L"hello Song-Song");
    CHECK(compiled.compiled.references("name"));
    CHECK_FALSE(compiled.compiled.references("artist"));
    CHECK_FALSE(compiled.compiled.references("album"));
    CHECK_FALSE(compiled.compiled.references("duration"));
}

TEST_CASE("iTunes song format rejects invalid templates", "[itunes][format][unit]") {
    CHECK_FALSE(song::compile_utf8("").ok);
    CHECK_FALSE(song::compile_utf8("{name").ok);
    CHECK_FALSE(song::compile_utf8("{}").ok);
    CHECK_FALSE(song::compile_utf8("{ name }").ok);
    CHECK_FALSE(song::compile_utf8("line\nstill").ok);
    CHECK_FALSE(song::compile_utf8("{album_artist}").ok);
    CHECK_FALSE(song::compile_utf8("{composer}").ok);
    CHECK_FALSE(song::compile_utf8("{rating}").ok);
    CHECK(song::compile_utf8("{album_artist}").error.find("Unknown") != std::string::npos);
}

TEST_CASE("iTunes song format strips one trailing newline", "[itunes][format][unit]") {
    const auto compiled = song::compile_utf8("[{name}]\n");
    REQUIRE(compiled.ok);
    CHECK(render(compiled.compiled) == L"[Song]");
    CHECK_FALSE(song::compile_utf8("[{name}]\n[{artist}]\n").ok);
}

TEST_CASE("Invalid iTunes song format does not produce replacement bytes", "[itunes][format][unit]") {
    CHECK_FALSE(song::accepted_song_format("{album_artist}").has_value());
    CHECK_FALSE(song::accepted_song_format(std::string(4097, 'a')).has_value());
    CHECK_FALSE(song::accepted_song_format("\xff").has_value());
    CHECK(song::accepted_song_format("[{name}]\n") == "[{name}]");

    const auto invalid = song::compile_utf8("{year}");
    CHECK_FALSE(invalid.ok);
    CHECK(render(invalid.compiled) == L"[Song] [Artist] [Album] [3:42]");
}

TEST_CASE("Missing iTunes song.format uses the compiled default", "[itunes][format][unit]") {
    const auto missing = std::filesystem::temp_directory_path() /
        "ac_itunes_missing_song_format" / "song.format";
    std::error_code error;
    std::filesystem::remove_all(missing.parent_path(), error);

    const auto loaded = song::load_song_format_file(missing);

    CHECK_FALSE(loaded.invalid);
    CHECK(loaded.error.empty());
    CHECK(render(loaded.compiled) == L"[Song] [Artist] [Album] [3:42]");
    CHECK_FALSE(std::filesystem::exists(missing));
}

TEST_CASE("Supported iTunes format tokens are the four verified fields", "[itunes][format][unit]") {
    REQUIRE(std::size(metadata::catalog) == 4);
    CHECK(metadata::catalog[0].token == "name");
    CHECK(std::wstring {metadata::catalog[0].com_property} == L"Name");
    CHECK(metadata::catalog[0].kind == metadata::ValueKind::bstr);
    CHECK(metadata::catalog[1].token == "artist");
    CHECK(metadata::catalog[2].token == "album");
    CHECK(metadata::catalog[3].token == "duration");
    CHECK(metadata::catalog[3].kind == metadata::ValueKind::duration_seconds);
    CHECK(std::wstring {metadata::catalog[3].com_property} == L"Duration");
}
