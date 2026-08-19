#include "catch_amalgamated.hpp"

import auto_core.encoding;

TEST_CASE("UTF-16 text converts to UTF-8", "[encoding][unit]") {
    CHECK(ac::encoding::to_utf8(L"") == "");
    CHECK(ac::encoding::to_utf8(L"Auto Core") == "Auto Core");
    CHECK(ac::encoding::to_utf8(L"caf\u00e9") == "caf\xc3\xa9");
    CHECK(ac::encoding::to_utf8(L"\U0001f600") == "\xf0\x9f\x98\x80");
}

TEST_CASE("A single UTF-16 code unit converts to UTF-8", "[encoding][unit]") {
    CHECK(ac::encoding::to_utf8(L'A') == "A");
    CHECK(ac::encoding::to_utf8(L'\u00e9') == "\xc3\xa9");
}

TEST_CASE("UTF-8 text converts to UTF-16", "[encoding][unit]") {
    CHECK(ac::encoding::to_utf16("") == L"");
    CHECK(ac::encoding::to_utf16("Auto Core") == L"Auto Core");
    CHECK(ac::encoding::to_utf16("caf\xc3\xa9") == L"caf\u00e9");
    CHECK(ac::encoding::to_utf16("\xf0\x9f\x98\x80") == L"\U0001f600");
}

TEST_CASE("Conversions preserve embedded null characters", "[encoding][unit]") {
    const std::wstring utf16 {L'a', L'\0', L'b'};
    const std::string utf8 {'a', '\0', 'b'};

    CHECK(ac::encoding::to_utf8(utf16) == utf8);
    CHECK(ac::encoding::to_utf16(utf8) == utf16);
}

TEST_CASE("Valid text round-trips through both encodings", "[encoding][unit]") {
    const std::wstring utf16 = L"Auto Core: caf\u00e9 \U0001f600";
    const std::string utf8 = "Auto Core: caf\xc3\xa9 \xf0\x9f\x98\x80";

    CHECK(ac::encoding::to_utf16(ac::encoding::to_utf8(utf16)) == utf16);
    CHECK(ac::encoding::to_utf8(ac::encoding::to_utf16(utf8)) == utf8);
}

TEST_CASE("Malformed UTF-16 is rejected", "[encoding][unit]") {
    const wchar_t high_surrogate = static_cast<wchar_t>(0xd800);
    const wchar_t low_surrogate = static_cast<wchar_t>(0xdc00);

    CHECK_THROWS_AS(
        ac::encoding::to_utf8(high_surrogate),
        std::runtime_error
    );
    CHECK_THROWS_AS(
        ac::encoding::to_utf8(low_surrogate),
        std::runtime_error
    );
    CHECK_THROWS_AS(
        ac::encoding::to_utf8(std::wstring_view {&high_surrogate, 1}),
        std::runtime_error
    );
}

TEST_CASE(
    "Malformed UTF-8 is rejected",
    "[encoding][unit]"
) {
    const std::pair<std::string_view, std::string> cases[] {
        {"invalid leading byte", std::string {static_cast<char>(0xff)}},
        {"isolated continuation byte", std::string {static_cast<char>(0x80)}},
        {
            "overlong encoding",
            std::string {static_cast<char>(0xc0), static_cast<char>(0xaf)}
        },
        {
            "truncated sequence",
            std::string {static_cast<char>(0xe2), static_cast<char>(0x82)}
        },
        {
            "UTF-8 encoded surrogate",
            std::string {
                static_cast<char>(0xed),
                static_cast<char>(0xa0),
                static_cast<char>(0x80)
            }
        },
        {
            "code point above U+10FFFF",
            std::string {
                static_cast<char>(0xf4),
                static_cast<char>(0x90),
                static_cast<char>(0x80),
                static_cast<char>(0x80)
            }
        }
    };

    for (const auto& [description, malformed_utf8] : cases) {
        INFO(description);
        CHECK_THROWS_AS(
            ac::encoding::to_utf16(malformed_utf8),
            std::runtime_error
        );
    }
}
