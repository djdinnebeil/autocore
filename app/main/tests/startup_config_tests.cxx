#include "catch_amalgamated.hpp"

import auto_core.main.defaults;
import auto_core_initialization;

namespace defaults = ac::main::defaults;
namespace initialization = ac::main::init;

TEST_CASE("Startup configuration defaults are sectionless and enabled", "[startup-config][unit]") {
    CHECK(defaults::startup_show_day_of_week);
    CHECK(defaults::startup_show_task_list);
    CHECK(defaults::ini_for_startup() ==
        "show_day_of_week = on\n"
        "show_task_list = on\n");
}

TEST_CASE("Startup configuration serializes settings independently", "[startup-config][unit]") {
    CHECK(defaults::ini_for_startup(true, false) ==
        "show_day_of_week = on\n"
        "show_task_list = off\n");
    CHECK(defaults::ini_for_startup(false, true) ==
        "show_day_of_week = off\n"
        "show_task_list = on\n");
    CHECK(defaults::ini_for_startup(false, false) ==
        "show_day_of_week = off\n"
        "show_task_list = off\n");
}

TEST_CASE("Startup configuration participates in Main initialization", "[startup-config][unit]") {
    const auto& steps = initialization::initialization_steps;
    REQUIRE(steps.size() == 9);
    CHECK(steps[4].executable == "startup_config.exe");
    CHECK(steps[4].kind == initialization::OwnerKind::configured);
    CHECK(initialization::arguments_for(
        steps[4].kind,
        initialization::InitializationMode::seed
    ) == L"--seed");
    CHECK(initialization::arguments_for(
        steps[4].kind,
        initialization::InitializationMode::interactive
    ) == L"--init");
    CHECK(initialization::arguments_for(
        steps[4].kind,
        initialization::InitializationMode::seeded_interactive
    ) == L"--init --seed");
}
