#include "catch_amalgamated.hpp"

#include <string>

import slash_defaults;
import slash_config_detail;

using slash::config::Action;

TEST_CASE("Slash seed takes precedence and preserves an existing file", "[slash][config]") {
    CHECK(slash::config::select_action(false, false, true) == Action::seed);
    CHECK(slash::config::select_action(false, true, true) == Action::seed);
    CHECK(slash::config::select_action(true, true, true) == Action::skip_seed);
    CHECK(slash::config::select_action(true, false, true) == Action::skip_seed);
    CHECK(std::string {slash::defaults::ini_text} ==
        "[slash]\n"
        "mode = verbose\n"
        "logging = on\n");
}

TEST_CASE("Slash initialization is separate from an existing configuration", "[slash][config]") {
    CHECK(slash::config::select_action(false, true, false) == Action::initialize);
    CHECK(slash::config::select_action(false, false, false) == Action::initialize);
    CHECK(slash::config::select_action(true, true, false) ==
        Action::skip_initialization);
    CHECK(slash::config::select_action(true, false, false) == Action::configure);
}

TEST_CASE("Slash mode values stay the closed set", "[slash][config]") {
    CHECK(slash::defaults::is_mode("verbose"));
    CHECK(slash::defaults::is_mode("concise"));
    CHECK(slash::defaults::is_mode("silent"));
    CHECK_FALSE(slash::defaults::is_mode("detailed"));
    CHECK(slash::defaults::ini_for_mode("silent", true) ==
        "[slash]\nmode = silent\nlogging = on\n");
}
