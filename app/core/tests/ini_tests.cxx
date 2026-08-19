#include "catch_amalgamated.hpp"

import auto_core.ini;

TEST_CASE("INI parser reads sectioned settings", "[ini][unit]") {
    const auto document = ac::ini::parse(
        "[app]\r\n"
        " program_title = Auto Core \r\n"
        "empty =   \r\n"
        "expression = left=right\r\n"
    );

    REQUIRE(document.find("app", "program_title"));
    CHECK(*document.find("app", "program_title") == "Auto Core");
    REQUIRE(document.find("app", "empty"));
    CHECK(document.find("app", "empty")->empty());
    CHECK(*document.find("app", "expression") == "left=right");
}

TEST_CASE("INI parser ignores comments and malformed input", "[ini][unit]") {
    const auto document = ac::ini::parse(
        "outside = ignored\n"
        "; comment\n"
        "# comment\n"
        "[logger]\n"
        "malformed\n"
        "enabled = true\n"
    );

    CHECK_FALSE(document.find("", "outside"));
    CHECK_FALSE(document.find("logger", "malformed"));
    CHECK(*document.find("logger", "enabled") == "true");
}

TEST_CASE("INI parser is case-sensitive", "[ini][unit]") {
    const auto document = ac::ini::parse(
        "[Logger]\nEnabled = true\n"
    );

    CHECK(document.find("Logger", "Enabled"));
    CHECK_FALSE(document.find("logger", "Enabled"));
    CHECK_FALSE(document.find("Logger", "enabled"));
}

TEST_CASE("Last duplicate INI setting wins", "[ini][unit]") {
    const auto document = ac::ini::parse(
        "[server]\nport = 8000\nport = 8585\n"
    );

    CHECK(*document.find("server", "port") == "8585");
}

TEST_CASE("INI sections can be revisited", "[ini][unit]") {
    const auto document = ac::ini::parse(
        "[one]\nvalue = first\n"
        "[two]\nvalue = second\n"
        "[one]\nvalue = final\n"
    );

    CHECK(*document.find("one", "value") == "final");
    CHECK(*document.find("two", "value") == "second");
}
