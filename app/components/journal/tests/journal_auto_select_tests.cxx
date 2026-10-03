#include "catch_amalgamated.hpp"

import journal_auto_select;

TEST_CASE("auto_select_new_series defaults on and accepts on or off", "[journal][unit]") {
    CHECK(journal::auto_select::enabled(std::nullopt));
    CHECK(journal::auto_select::enabled(std::string_view {"on"}));
    CHECK(journal::auto_select::enabled(std::string_view {" ON "}));
    CHECK_FALSE(journal::auto_select::enabled(std::string_view {"off"}));
    CHECK_FALSE(journal::auto_select::enabled(std::string_view {"OFF"}));
    CHECK(journal::auto_select::enabled(std::string_view {"enable"}));
    CHECK(journal::auto_select::enabled(std::string_view {}));
}
