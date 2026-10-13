#include "catch_amalgamated.hpp"
#include "../config/core_config_detail.hpp"

namespace detail = ac::config::detail;

TEST_CASE("Warnings stay on by default", "[core-config][unit]") {
    const auto settings = detail::resolve({});

    CHECK(settings.missing_winkey_mapping);
    CHECK(settings.unmapped_key);
}

TEST_CASE("Warnings stay on when set to on", "[core-config][unit]") {
    const auto settings = detail::resolve({
        .missing_winkey_mapping = "on",
        .unmapped_key = "on"
    });

    CHECK(settings.missing_winkey_mapping);
    CHECK(settings.unmapped_key);
}

TEST_CASE("Warnings turn off only for exact off", "[core-config][unit]") {
    const auto settings = detail::resolve({
        .missing_winkey_mapping = "off",
        .unmapped_key = "off"
    });

    CHECK_FALSE(settings.missing_winkey_mapping);
    CHECK_FALSE(settings.unmapped_key);
}

TEST_CASE("Each warning resolves independently", "[core-config][unit]") {
    const auto settings = detail::resolve({
        .missing_winkey_mapping = "off",
        .unmapped_key = "on"
    });

    CHECK_FALSE(settings.missing_winkey_mapping);
    CHECK(settings.unmapped_key);
}

TEST_CASE("Unrecognized warning values stay on", "[core-config][unit]") {
    const auto settings = detail::resolve({
        .missing_winkey_mapping = "no",
        .unmapped_key = "yes"
    });

    CHECK(settings.missing_winkey_mapping);
    CHECK(settings.unmapped_key);
}
