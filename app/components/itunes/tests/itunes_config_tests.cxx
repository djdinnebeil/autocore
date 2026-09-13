#include "catch_amalgamated.hpp"
#include "../itunes_config_detail.hpp"

namespace detail = itunes::config::detail;

TEST_CASE("iTunes configuration uses component defaults", "[itunes][config][unit]") {
    const auto settings = detail::resolve({});

    CHECK_FALSE(settings.auto_start);
    CHECK(settings.tab_end == 3);
}

TEST_CASE("iTunes configuration resolves explicit values", "[itunes][config][unit]") {
    const auto settings = detail::resolve({
        .auto_start = "true",
        .tab_end = "5"
    });

    CHECK(settings.auto_start);
    CHECK(settings.tab_end == 5);
}

TEST_CASE("iTunes auto_start accepts only lowercase true and false", "[itunes][config][unit]") {
    CHECK_FALSE(detail::resolve({.auto_start = "false"}).auto_start);
    CHECK(detail::resolve({.auto_start = "true"}).auto_start);

    CHECK_FALSE(detail::resolve({.auto_start = "TRUE"}).auto_start);
    CHECK_FALSE(detail::resolve({.auto_start = "1"}).auto_start);
    CHECK(detail::resolve({.auto_start = "TRUE"}, {.auto_start = true})
        .auto_start);
    CHECK_FALSE(detail::resolve({.auto_start = "false"}, {.auto_start = true})
        .auto_start);
}

TEST_CASE("Malformed iTunes tab limits preserve the prior value", "[itunes][config][unit]") {
    const detail::Settings defaults {
        .auto_start = true,
        .tab_end = 7
    };

    CHECK(detail::resolve({.tab_end = ""}, defaults).tab_end == 7);
    CHECK(detail::resolve({.tab_end = "4x"}, defaults).tab_end == 7);
    CHECK(detail::resolve({.tab_end = "three"}, defaults).tab_end == 7);
}

TEST_CASE("Parsed iTunes tab limits retain current integer semantics", "[itunes][config][unit]") {
    CHECK(detail::resolve({.tab_end = "0"}).tab_end == 0);
    CHECK(detail::resolve({.tab_end = "-2"}).tab_end == -2);
}
