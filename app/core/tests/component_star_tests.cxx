#include "catch_amalgamated.hpp"

import component_star;

TEST_CASE("Star menu uses Configure when the INI exists", "[component-star][unit]") {
    const auto entries = ac::component_star::menu_entries({
        .configuration_present = true,
        .enabled = true
    });

    REQUIRE(entries.size() == 3);
    CHECK(entries[0].kind == ac::component_star::MenuKind::configure);
    CHECK(entries[0].label == "Configure");
    CHECK(entries[1].kind == ac::component_star::MenuKind::toggle);
    CHECK(entries[1].label == "Disable");
    CHECK(entries[2].kind == ac::component_star::MenuKind::exit_star);
    CHECK(entries[2].label == "Exit");
}

TEST_CASE("Star menu uses Initialize when the INI is missing", "[component-star][unit]") {
    const auto entries = ac::component_star::menu_entries({
        .configuration_present = false,
        .enabled = false
    });

    REQUIRE(entries.size() == 3);
    CHECK(entries[0].kind == ac::component_star::MenuKind::initialize);
    CHECK(entries[0].label == "Initialize");
    CHECK(entries[1].label == "Enable");
    CHECK(entries[2].label == "Exit");
}

TEST_CASE(
    "Star menu inserts delegated actions before Enable and Exit",
    "[component-star][unit]"
) {
    const ac::component_star::DelegatedAction actions[] {
        {"OAuth", "spotify_oauth.exe"},
        {"Database manager", "journal_db.exe"}
    };
    const auto entries = ac::component_star::menu_entries({
        .configuration_present = true,
        .enabled = false,
        .delegated = actions
    });

    REQUIRE(entries.size() == 5);
    CHECK(entries[0].label == "Configure");
    CHECK(entries[1].kind == ac::component_star::MenuKind::delegated);
    CHECK(entries[1].label == "OAuth");
    CHECK(entries[1].delegated_index == 0);
    CHECK(entries[2].label == "Database manager");
    CHECK(entries[2].delegated_index == 1);
    CHECK(entries[3].kind == ac::component_star::MenuKind::toggle);
    CHECK(entries[3].label == "Enable");
    CHECK(entries[4].kind == ac::component_star::MenuKind::exit_star);
    CHECK(entries[4].label == "Exit");
}

TEST_CASE(
    "Star menu offers Complete initialization only for an incomplete baseline",
    "[component-star][unit]"
) {
    const ac::component_star::DelegatedAction actions[] {
        {"Edit port and document root", "server_editor.exe"},
        {"Create default site files", "server_builder.exe", {"--seed"}}
    };
    const auto incomplete = ac::component_star::menu_entries({
        .configuration_present = true,
        .enabled = true,
        .initialization_incomplete = true,
        .delegated = actions
    });

    REQUIRE(incomplete.size() == 6);
    CHECK(incomplete[0].kind == ac::component_star::MenuKind::configure);
    CHECK(incomplete[0].label == "Configure");
    CHECK(
        incomplete[1].kind ==
        ac::component_star::MenuKind::complete_initialization
    );
    CHECK(incomplete[1].label == "Complete initialization");
    CHECK(incomplete[2].label == "Edit port and document root");
    CHECK(incomplete[3].label == "Create default site files");
    CHECK(incomplete[4].label == "Disable");
    CHECK(incomplete[5].label == "Exit");

    const auto ready = ac::component_star::menu_entries({
        .configuration_present = true,
        .enabled = true,
        .initialization_incomplete = false,
        .delegated = actions
    });
    REQUIRE(ready.size() == 5);
    CHECK(ready[0].label == "Configure");
    CHECK(ready[1].label == "Edit port and document root");

    const auto missing_ini = ac::component_star::menu_entries({
        .configuration_present = false,
        .enabled = false,
        .initialization_incomplete = true,
        .delegated = actions
    });
    REQUIRE(missing_ini.size() == 5);
    CHECK(missing_ini[0].kind == ac::component_star::MenuKind::initialize);
    CHECK(missing_ini[0].label == "Initialize");
    CHECK(missing_ini[1].label == "Edit port and document root");
}
