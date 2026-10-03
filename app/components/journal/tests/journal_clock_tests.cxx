#include "catch_amalgamated.hpp"

import journal_extended_hours;

namespace {

journal::extended_hours::ExtendedHours token(const std::string_view text) {
    const auto parsed = journal::extended_hours::parse_token(text);
    REQUIRE(parsed.has_value());
    return *parsed;
}

void expect(
    const std::string_view text,
    const int hour,
    const int minute,
    const std::string_view formatted
) {
    CHECK(
        journal::extended_hours::format(token(text), hour, minute) ==
        formatted
    );
}

} // namespace

TEST_CASE("Inclusive zero is the extended-hour seed token", "[journal][unit]") {
    CHECK(journal::extended_hours::file_text({0, true}) == "extended_hours = +0\n");
    CHECK(journal::extended_hours::canonical_token({0, true}) == "+0");
}

TEST_CASE(
    "Extended-hour tokens keep the cutoff and the boundary flag distinct",
    "[journal][unit]"
) {
    const auto disabled = token("0");
    const auto midnight = token("+0");
    const auto exclusive = token("3");
    const auto inclusive = token("+3");

    CHECK(disabled.cutoff_hour == 0);
    CHECK_FALSE(disabled.include_boundary);
    CHECK(midnight.cutoff_hour == 0);
    CHECK(midnight.include_boundary);
    CHECK(disabled != midnight);

    CHECK(exclusive.cutoff_hour == 3);
    CHECK_FALSE(exclusive.include_boundary);
    CHECK(inclusive.cutoff_hour == 3);
    CHECK(inclusive.include_boundary);
    CHECK(exclusive != inclusive);

    CHECK(journal::extended_hours::canonical_token(exclusive) == "3");
    CHECK(journal::extended_hours::canonical_token(inclusive) == "+3");
    CHECK(journal::extended_hours::file_text(exclusive) == "extended_hours = 3\n");
    CHECK(journal::extended_hours::file_text(inclusive) == "extended_hours = +3\n");
}

TEST_CASE(
    "Extended-hour formatting follows the cutoff token",
    "[journal][unit]"
) {
    expect("0", 0, 0, "00:00");

    expect("+0", 0, 0, "24:00");
    expect("+0", 0, 1, "00:01");

    expect("1", 0, 59, "24:59");
    expect("1", 1, 0, "01:00");

    expect("+1", 0, 59, "24:59");
    expect("+1", 1, 0, "25:00");
    expect("+1", 1, 1, "01:01");

    expect("2", 1, 35, "25:35");
    expect("2", 2, 0, "02:00");
    expect("+2", 1, 35, "25:35");
    expect("+2", 2, 0, "26:00");

    expect("3", 2, 59, "26:59");
    expect("3", 3, 0, "03:00");

    expect("+3", 2, 59, "26:59");
    expect("+3", 3, 0, "27:00");
    expect("+3", 3, 1, "03:01");

    expect("12", 11, 59, "35:59");
    expect("12", 12, 0, "12:00");

    expect("+12", 11, 59, "35:59");
    expect("+12", 12, 0, "36:00");
    expect("+12", 12, 1, "12:01");
}

TEST_CASE(
    "Extended-hour parser rejects malformed tokens",
    "[journal][unit]"
) {
    const std::string_view rejected[] {
        "-1",
        "13",
        "+13",
        "+01",
        "01",
        "++",
        "+",
        "abc"
    };
    for (const auto text : rejected) {
        INFO(text);
        CHECK_FALSE(journal::extended_hours::parse_token(text).has_value());
    }
}

TEST_CASE(
    "Extended-hour clock file keeps one literal token",
    "[journal][unit]"
) {
    const auto exclusive = journal::extended_hours::parse_file(
        "extended_hours = 3\n"
    );
    const auto inclusive = journal::extended_hours::parse_file(
        "extended_hours = +3\n"
    );
    REQUIRE(exclusive.has_value());
    REQUIRE(inclusive.has_value());
    CHECK(*exclusive != *inclusive);
    CHECK_FALSE(journal::extended_hours::parse_file(
        "[timestamp]\nextended_hours = 3\n"
    ).has_value());
    CHECK_FALSE(journal::extended_hours::parse_file(
        "extended_hours = 01\n"
    ).has_value());
}
