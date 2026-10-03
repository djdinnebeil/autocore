#include "catch_amalgamated.hpp"

import journal_remote_sync;

TEST_CASE("remote_sync accepts on and off only", "[journal][unit]") {
    CHECK(journal::remote_sync::enabled("on"));
    CHECK(journal::remote_sync::enabled(" ON "));
    CHECK(journal::remote_sync::enabled("On"));
    CHECK(journal::remote_sync::canonical("on") == "on");
    CHECK(journal::remote_sync::canonical(" OFF ") == "off");
    CHECK_FALSE(journal::remote_sync::enabled("off"));
    CHECK_FALSE(journal::remote_sync::enabled("OFF"));
    CHECK_FALSE(journal::remote_sync::enabled("enable"));
    CHECK_FALSE(journal::remote_sync::enabled("enabled"));
    CHECK_FALSE(journal::remote_sync::enabled("disable"));
    CHECK_FALSE(journal::remote_sync::enabled(""));
    CHECK_FALSE(journal::remote_sync::enabled("true"));
    CHECK_FALSE(journal::remote_sync::canonical("enable").has_value());
    CHECK_FALSE(journal::remote_sync::canonical("disable").has_value());
}
