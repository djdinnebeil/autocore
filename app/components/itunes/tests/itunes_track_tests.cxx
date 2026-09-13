#include "catch_amalgamated.hpp"
#include "../itunes_track_detail.hpp"

namespace detail = itunes::track::detail;

TEST_CASE("iTunes track formatting includes metadata and duration", "[itunes][track][unit]") {
    CHECK(
        detail::format_track(L"Song", L"Artist", L"Album", 245) ==
        L"[Song] [Artist] [Album] [4:05]"
    );
    CHECK(
        detail::format_track(L"Short", L"Artist", L"Album", 5) ==
        L"[Short] [Artist] [Album] [0:05]"
    );
}

TEST_CASE("iTunes track formatting preserves empty metadata", "[itunes][track][unit]") {
    CHECK(
        detail::format_track(L"", L"", L"", 0) ==
        L"[] [] [] [0:00]"
    );
}

TEST_CASE("iTunes history records only track changes", "[itunes][history][unit]") {
    std::vector<std::wstring> history;
    std::wstring last_track;

    CHECK(detail::record_history(history, last_track, L"First"));
    CHECK_FALSE(detail::record_history(history, last_track, L"First"));
    CHECK(detail::record_history(history, last_track, L"Second"));

    REQUIRE(history.size() == 2);
    CHECK(history[0] == L"First");
    CHECK(history[1] == L"Second");
    CHECK(last_track == L"Second");
}

TEST_CASE("Draining iTunes history formats and clears entries", "[itunes][history][unit]") {
    std::vector<std::wstring> history {L"First", L"Second"};

    CHECK(detail::drain_history(history) == L"First\nSecond\n");
    CHECK(history.empty());
}

TEST_CASE("Empty iTunes history produces the current newline marker", "[itunes][history][unit]") {
    std::vector<std::wstring> history;

    CHECK(detail::drain_history(history) == L"\n");
    CHECK(history.empty());
}
