#include "catch_amalgamated.hpp"

import journal_episode_format;

TEST_CASE("Episode numbers pad without truncating", "[journal][unit]") {
    CHECK(journal::format_episode_number(9, 0) == "9");
    CHECK(journal::format_episode_number(9, 1) == "9");
    CHECK(journal::format_episode_number(9, 2) == "09");
    CHECK(journal::format_episode_number(9, 3) == "009");
    CHECK(journal::format_episode_number(42, 2) == "42");
    CHECK(journal::format_episode_number(42, 3) == "042");
    CHECK(journal::format_episode_number(123, 2) == "123");
}
