#include "catch_amalgamated.hpp"
#include "../config/core_config_detail.hpp"

namespace detail = ac::config::detail;

TEST_CASE("Core configuration keeps the winkey warning by default", "[core-config][unit]") {
    const auto settings = detail::resolve({});

    CHECK(settings.warn_without_winkey_mapping);
}

TEST_CASE("Core configuration keeps the winkey warning when on", "[core-config][unit]") {
    const auto settings = detail::resolve({
        .warn_without_winkey_mapping = "on"
    });

    CHECK(settings.warn_without_winkey_mapping);
}

TEST_CASE("Core configuration silences the winkey warning only for off", "[core-config][unit]") {
    const auto settings = detail::resolve({
        .warn_without_winkey_mapping = "off"
    });

    CHECK_FALSE(settings.warn_without_winkey_mapping);
}

TEST_CASE("Core configuration keeps the winkey warning for unrecognized values", "[core-config][unit]") {
    CHECK(detail::resolve({
        .warn_without_winkey_mapping = "no"
    }).warn_without_winkey_mapping);
    CHECK(detail::resolve({
        .warn_without_winkey_mapping = "yes"
    }).warn_without_winkey_mapping);
}
