#include "catch_amalgamated.hpp"
#include "../src/spotify_config_detail.hpp"

namespace detail = ac::spotify::config::detail;

TEST_CASE(
    "Spotify directory uses an explicit absolute path",
    "[spotify-config][unit]"
) {
    const auto directory = detail::resolve(
        {.directory = R"(D:\spotify-data)"},
        R"(C:\default-spotify)",
        R"(C:\app)"
    );

    CHECK(directory == std::filesystem::path {R"(D:\spotify-data)"});
}

TEST_CASE(
    "Missing Spotify directory uses the default",
    "[spotify-config][unit]"
) {
    const auto directory = detail::resolve(
        {},
        R"(C:\default-spotify)",
        R"(C:\app)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\default-spotify)"});
}

TEST_CASE(
    "Empty Spotify directory uses the default",
    "[spotify-config][unit]"
) {
    const auto directory = detail::resolve(
        {.directory = ""},
        R"(C:\default-spotify)",
        R"(C:\app)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\default-spotify)"});
}

TEST_CASE(
    "Relative Spotify directory joins the executable directory",
    "[spotify-config][unit]"
) {
    const auto directory = detail::resolve(
        {.directory = "spotify"},
        R"(C:\default-spotify)",
        R"(C:\app)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\app\spotify)"});
}

TEST_CASE(
    "Relative Spotify directory is lexically normalized",
    "[spotify-config][unit]"
) {
    const auto directory = detail::resolve(
        {.directory = R"(spotify\cache\..\data)"},
        R"(C:\default-spotify)",
        R"(C:\app)"
    );

    CHECK(directory == std::filesystem::path {R"(C:\app\spotify\data)"});
}
