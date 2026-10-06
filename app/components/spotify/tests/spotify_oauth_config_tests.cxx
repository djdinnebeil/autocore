#include <catch_amalgamated.hpp>

import spotify_defaults;

using spotify::defaults::AuthorizationCondition;

TEST_CASE("Missing Spotify auto_launch_oauth stays off", "[spotify][oauth][unit]") {
    CHECK_FALSE(spotify::defaults::auto_launch_enabled(""));
    CHECK_FALSE(spotify::defaults::auto_launch_enabled("maybe"));
    CHECK_FALSE(spotify::defaults::auto_launch_enabled("ON"));
}

TEST_CASE("Spotify auto_launch_oauth accepts on and off aliases", "[spotify][oauth][unit]") {
    CHECK(spotify::defaults::auto_launch_enabled("on"));
    CHECK_FALSE(spotify::defaults::auto_launch_enabled("true"));
    CHECK_FALSE(spotify::defaults::auto_launch_enabled("off"));
    CHECK_FALSE(spotify::defaults::auto_launch_enabled("false"));
}

TEST_CASE("Compiled Spotify defaults include auto_launch_oauth", "[spotify][oauth][unit]") {
    const std::string text {spotify::defaults::ini_text};
    CHECK(text.find("directory = components\\spotify\n") != std::string::npos);
    CHECK(text.find("auto_launch_oauth = off\n") != std::string::npos);
    CHECK(spotify::defaults::ini_for("") == text);
    CHECK(spotify::defaults::ini_for(spotify::defaults::directory, false) == text);
    CHECK(
        spotify::defaults::ini_for("components\\library", false) ==
        "[spotify]\n"
        "directory = components\\library\n"
        "auto_launch_oauth = off\n"
        "logging = on\n"
    );
    CHECK(
        spotify::defaults::ini_for(spotify::defaults::directory, true) ==
        "[spotify]\n"
        "directory = components\\spotify\n"
        "auto_launch_oauth = on\n"
        "logging = on\n"
    );
}

TEST_CASE("Spotify OAuth launches only for a new interactive failure", "[spotify][oauth][unit]") {
    const AuthorizationCondition quiet[] {
        AuthorizationCondition::usable_access_token,
        AuthorizationCondition::refreshable,
        AuthorizationCondition::warning_only,
        AuthorizationCondition::transient_failure
    };
    for (const auto condition : quiet) {
        CHECK_FALSE(spotify::defaults::should_auto_launch_oauth(
            condition,
            true,
            false
        ));
    }

    CHECK(spotify::defaults::should_auto_launch_oauth(
        AuthorizationCondition::interactive_required,
        true,
        false
    ));
    CHECK_FALSE(spotify::defaults::should_auto_launch_oauth(
        AuthorizationCondition::interactive_required,
        false,
        false
    ));
    CHECK_FALSE(spotify::defaults::should_auto_launch_oauth(
        AuthorizationCondition::interactive_required,
        true,
        true
    ));
}

TEST_CASE("Run-now answer is independent of stored auto_launch_oauth", "[spotify][oauth][unit]") {
    const auto declined = spotify::defaults::accepted_yes_no("n");
    REQUIRE(declined.has_value());
    CHECK_FALSE(*declined);
    CHECK(spotify::defaults::auto_launch_enabled("on"));
    CHECK(spotify::defaults::ini_for("components\\spotify", true).find(
        "auto_launch_oauth = on\n"
    ) != std::string::npos);

    const auto accepted = spotify::defaults::accepted_yes_no("");
    REQUIRE(accepted.has_value());
    CHECK(*accepted);
    CHECK_FALSE(spotify::defaults::auto_launch_enabled("off"));
    CHECK(spotify::defaults::ini_for("components\\spotify", false).find(
        "auto_launch_oauth = off\n"
    ) != std::string::npos);

    CHECK_FALSE(spotify::defaults::accepted_yes_no("later").has_value());
    const auto yes = spotify::defaults::accepted_yes_no("Y");
    const auto no = spotify::defaults::accepted_yes_no("N");
    REQUIRE(yes.has_value());
    REQUIRE(no.has_value());
    CHECK(*yes);
    CHECK_FALSE(*no);
}
