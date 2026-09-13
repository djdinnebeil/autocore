// Spotify HTTP response classification tests.
#include "catch_amalgamated.hpp"

import spotify_http;

TEST_CASE("Spotify accepts every successful HTTP status", "[spotify][http][unit]") {
    CHECK_FALSE(spotify_http::is_success_status(199));
    CHECK(spotify_http::is_success_status(200));
    CHECK(spotify_http::is_success_status(204));
    CHECK(spotify_http::is_success_status(299));
    CHECK_FALSE(spotify_http::is_success_status(300));
}
