#include <catch_amalgamated.hpp>

import spotify_application_data;
import spotify_token_store;

TEST_CASE("Missing or empty client.id text is absent", "[spotify][data][unit]") {
    CHECK_FALSE(spotify::data::parse_client_id("").has_value());
    CHECK_FALSE(spotify::data::parse_client_id("\n").has_value());
}

TEST_CASE("Whitespace-only client.id text is absent", "[spotify][data][unit]") {
    CHECK_FALSE(spotify::data::parse_client_id(" \t\r\n").has_value());
}

TEST_CASE("A valid client.id is the trimmed file text", "[spotify][data][unit]") {
    const auto id = spotify::data::parse_client_id("  client-id  \n");
    REQUIRE(id.has_value());
    CHECK(*id == "client-id");
}

TEST_CASE("An embedded newline makes client.id absent", "[spotify][data][unit]") {
    CHECK_FALSE(spotify::data::parse_client_id("ab\ncd").has_value());
    CHECK_FALSE(spotify::data::parse_client_id("ab\r\ncd").has_value());
}

TEST_CASE("Missing or empty devices.list has no devices", "[spotify][data][unit]") {
    CHECK(spotify::data::parse_devices("").empty());
    CHECK(spotify::data::parse_devices("\n\n").empty());
}

TEST_CASE("devices.list skips lines that are not usable", "[spotify][data][unit]") {
    CHECK(spotify::data::parse_devices("desktop\n= id\nname =\n = \n").empty());
}

TEST_CASE("devices.list keeps one usable line", "[spotify][data][unit]") {
    const auto devices = spotify::data::parse_devices("Desktop = abc\n");
    REQUIRE(devices.size() == 1);
    CHECK(devices[0].name == "desktop");
    CHECK(devices[0].id == "abc");
}

TEST_CASE("devices.list keeps several usable lines", "[spotify][data][unit]") {
    const auto devices = spotify::data::parse_devices(
        "phone = one\nlaptop = two\n"
    );
    REQUIRE(devices.size() == 2);
    CHECK(devices[0].id == "one");
    CHECK(devices[1].name == "laptop");
    CHECK(devices[1].id == "two");
}

TEST_CASE("devices.list splits a name on the last equals sign", "[spotify][data][unit]") {
    const auto devices = spotify::data::parse_devices("room = a = device-id\n");
    REQUIRE(devices.size() == 1);
    CHECK(devices[0].name == "room = a");
    CHECK(devices[0].id == "device-id");
}

TEST_CASE("devices.list keeps the last id for a mixed-case name", "[spotify][data][unit]") {
    const auto devices = spotify::data::parse_devices(
        "Phone = first\n phone = second \nPHONE = last\n"
    );
    REQUIRE(devices.size() == 1);
    CHECK(devices[0].name == "phone");
    CHECK(devices[0].id == "last");
}

TEST_CASE("Missing or empty tokens.map is incomplete", "[spotify][tokens][unit]") {
    CHECK_FALSE(spotify::tokens::parse("").has_value());
    CHECK_FALSE(spotify::tokens::parse("\n").has_value());
}

TEST_CASE("tokens.map is incomplete when a required key is absent", "[spotify][tokens][unit]") {
    CHECK_FALSE(spotify::tokens::parse(
        "access_token = access\n"
        "authorized_at = 10\n"
        "refresh_expires_at = 20\n"
    ).has_value());
}

TEST_CASE("tokens.map is incomplete when a token is empty", "[spotify][tokens][unit]") {
    CHECK_FALSE(spotify::tokens::parse(
        "access_token =\n"
        "refresh_token = refresh\n"
        "authorized_at = 10\n"
        "refresh_expires_at = 20\n"
    ).has_value());
}

TEST_CASE("tokens.map is incomplete when a timestamp is not numeric", "[spotify][tokens][unit]") {
    CHECK_FALSE(spotify::tokens::parse(
        "access_token = access\n"
        "refresh_token = refresh\n"
        "authorized_at = tomorrow\n"
        "refresh_expires_at = 20\n"
    ).has_value());
}

TEST_CASE("A complete tokens.map round-trips", "[spotify][tokens][unit]") {
    const auto parsed = spotify::tokens::parse(
        "access_token = access\n"
        "refresh_token = refresh\n"
        "authorized_at = 10\n"
        "refresh_expires_at = 20\n"
    );
    REQUIRE(parsed.has_value());
    CHECK(parsed->access_token == "access");
    CHECK(parsed->refresh_token == "refresh");
    CHECK(parsed->authorized_at == 10);
    CHECK(parsed->refresh_expires_at == 20);

    const auto again = spotify::tokens::parse(spotify::tokens::serialize(*parsed));
    REQUIRE(again.has_value());
    CHECK(again->access_token == parsed->access_token);
    CHECK(again->refresh_token == parsed->refresh_token);
    CHECK(again->authorized_at == parsed->authorized_at);
    CHECK(again->refresh_expires_at == parsed->refresh_expires_at);
}
