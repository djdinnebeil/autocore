#include "catch_amalgamated.hpp"

import auto_core.formatting;

TEST_CASE("Formatting supports ordinary narrow arguments", "[formatting][unit]") {
    CHECK(ac::formatting::format("Auto {}", "Core") == "Auto Core");
    CHECK(ac::formatting::format("{:04}", 42) == "0042");
    CHECK(
        ac::formatting::format("{}: {} ({})", "port", 8080, true) ==
        "port: 8080 (true)"
    );
}

TEST_CASE("Formatting normalizes UTF-16 arguments", "[formatting][unit]") {
    const std::wstring string = L"caf\u00e9";
    const std::wstring_view view = L"\U0001f600";
    wchar_t mutable_text[] = L"Auto Core";

    CHECK(ac::formatting::format("{}", L'\u00e9') == "\xc3\xa9");
    CHECK(ac::formatting::format("{}", string) == "caf\xc3\xa9");
    CHECK(ac::formatting::format("{}", view) == "\xf0\x9f\x98\x80");
    CHECK(ac::formatting::format("{}", L"wide literal") == "wide literal");
    CHECK(ac::formatting::format("{}", mutable_text) == "Auto Core");
}

TEST_CASE("Formatting supports mixed narrow and wide text", "[formatting][unit]") {
    CHECK(
        ac::formatting::format(
            "{} | {} | {}",
            "UTF-8",
            L"caf\u00e9",
            42
        ) == "UTF-8 | caf\xc3\xa9 | 42"
    );
}

TEST_CASE("Formatting normalizes Windows paths", "[formatting][unit]") {
    const std::filesystem::path path {L"C:\\caf\u00e9\\Auto Core.txt"};

    CHECK(
        ac::formatting::format("Path: {}", path) ==
        "Path: C:\\caf\xc3\xa9\\Auto Core.txt"
    );
}

TEST_CASE("Null C strings have explicit output", "[formatting][unit]") {
    const char* narrow_text = nullptr;
    const wchar_t* wide_text = nullptr;

    CHECK(ac::formatting::format("{}", narrow_text) == "(null)");
    CHECK(ac::formatting::format("{}", wide_text) == "(null)");
    CHECK(
        ac::formatting::format("{} | {}", narrow_text, wide_text) ==
        "(null) | (null)"
    );
}

TEST_CASE("A null format string is rejected", "[formatting][unit]") {
    CHECK_THROWS_AS(
        ac::formatting::format(static_cast<const char*>(nullptr), 42),
        std::invalid_argument
    );
}

TEST_CASE("Invalid format usage is rejected", "[formatting][unit]") {
    CHECK_THROWS_AS(ac::formatting::format("{"), std::format_error);
    CHECK_THROWS_AS(ac::formatting::format("{} {}", 1), std::format_error);
    CHECK_THROWS_AS(ac::formatting::format("{:d}", "text"), std::format_error);
}

TEST_CASE("Malformed UTF-16 arguments are rejected", "[formatting][unit]") {
    const wchar_t high_surrogate = static_cast<wchar_t>(0xd800);

    CHECK_THROWS_AS(
        ac::formatting::format("{}", high_surrogate),
        std::runtime_error
    );
}
