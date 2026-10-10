#include "catch_amalgamated.hpp"

#include <sstream>
#include <string>

import config_menu;

namespace menu = ac::config_menu;

namespace {

constexpr std::string_view modes[] {"verbose", "concise", "silent"};
constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting settings[] {
    {
        .key = "directory",
        .display_name = "Directory",
        .summary = "Data directory.",
        .default_value = "components\\spotify",
        .constraint = "Enter a path.",
    },
    {
        .key = "mode",
        .display_name = "Mode",
        .summary = "Report style.",
        .default_value = "verbose",
        .choices = modes,
    },
    {
        .key = "interval",
        .display_name = "Interval",
        .summary = "0 disables the timer.",
        .default_value = "60",
        .constraint = "Enter 0 or a positive number.",
    },
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging.",
        .default_value = "on",
        .choices = on_off,
    },
};

struct Store {
    std::string directory {"components\\spotify"};
    std::string mode {"verbose"};
    std::string interval {"60"};
    std::string logging {"on"};
    bool fail {false};
};

[[nodiscard]] std::string current(const Store& store, const menu::Setting& setting) {
    if (setting.key == "directory") {
        return store.directory;
    }
    if (setting.key == "mode") {
        return store.mode;
    }
    if (setting.key == "interval") {
        return store.interval;
    }
    return store.logging;
}

[[nodiscard]] menu::ApplyResult apply(
    Store& store,
    const menu::Setting& setting,
    const std::string_view value
) {
    if (store.fail) {
        return menu::ApplyResult::failed;
    }
    if (setting.key == "interval" && value == "nope") {
        return menu::ApplyResult::invalid;
    }
    if (setting.key == "directory") {
        store.directory = std::string {value};
    }
    else if (setting.key == "mode") {
        store.mode = std::string {value};
    }
    else if (setting.key == "interval") {
        store.interval = std::string {value};
    }
    else {
        store.logging = std::string {value};
    }
    return menu::ApplyResult::stored;
}

} // namespace

TEST_CASE("Reference header lists only closed values", "[config-menu][unit]") {
    const auto header = menu::reference_header(settings);
    CHECK(header ==
        "# mode = verbose | concise | silent\n"
        "# logging = on | off\n"
        "\n");
    CHECK(header.find("# directory") == std::string::npos);
    CHECK(header.find("# interval") == std::string::npos);
    CHECK(menu::with_header(settings, "[slash]\nmode = verbose\n") ==
        header + "[slash]\nmode = verbose\n");
}

TEST_CASE("Main menu shows values and returns after a choice", "[config-menu][unit]") {
    Store store;
    std::istringstream input {"2\n3\n0\n"};
    std::ostringstream output;
    const auto result = menu::run_menu(
        "Slash Configuration",
        settings,
        [&](const menu::Setting& setting) { return current(store, setting); },
        [&](const menu::Setting& setting, const std::string_view value) {
            return apply(store, setting, value);
        },
        input,
        output
    );

    CHECK(result.ok);
    CHECK(result.changed);
    CHECK(store.mode == "silent");
    const auto shown = output.str();
    CHECK(shown.find("Slash Configuration\n") != std::string::npos);
    CHECK(shown.find("1. Directory  components\\spotify\n") != std::string::npos);
    CHECK(shown.find("2. Mode       verbose\n") != std::string::npos);
    CHECK(shown.find("\n0. Exit\n") != std::string::npos);
    CHECK(shown.find("\n0. Back\n") != std::string::npos);
    CHECK(shown.find("3. silent\n") != std::string::npos);
    CHECK(shown.find("2. Mode       silent\n") != std::string::npos);
}

TEST_CASE("Invalid menu input reprints the menu", "[config-menu][unit]") {
    Store store;
    std::istringstream input {"9\n0\n"};
    std::ostringstream output;
    const auto result = menu::run_menu(
        "Slash Configuration",
        settings,
        [&](const menu::Setting& setting) { return current(store, setting); },
        [&](const menu::Setting& setting, const std::string_view value) {
            return apply(store, setting, value);
        },
        input,
        output
    );

    CHECK(result.ok);
    CHECK_FALSE(result.changed);
    CHECK(output.str().find("Enter 0-4.\n") != std::string::npos);
}

TEST_CASE("Free-form zero is stored and blank input is rejected", "[config-menu][unit]") {
    Store store;
    std::istringstream input {"3\n1\n\n0\n0\n"};
    std::ostringstream output;
    const auto result = menu::run_menu(
        "Logger Configuration",
        settings,
        [&](const menu::Setting& setting) { return current(store, setting); },
        [&](const menu::Setting& setting, const std::string_view value) {
            return apply(store, setting, value);
        },
        input,
        output
    );

    CHECK(result.ok);
    CHECK(result.changed);
    CHECK(store.interval == "0");
    CHECK(output.str().find("1. Change value\n0. Back\n") != std::string::npos);
    CHECK(output.str().find("Enter a value.\n") != std::string::npos);
}

TEST_CASE("Invalid free-form input repeats the constraint", "[config-menu][unit]") {
    Store store;
    std::istringstream input {"3\n1\nnope\n60\n0\n"};
    std::ostringstream output;
    const auto result = menu::run_menu(
        "Logger Configuration",
        settings,
        [&](const menu::Setting& setting) { return current(store, setting); },
        [&](const menu::Setting& setting, const std::string_view value) {
            return apply(store, setting, value);
        },
        input,
        output
    );

    CHECK(result.ok);
    CHECK(store.interval == "60");
    CHECK(output.str().find("Enter 0 or a positive number.\n") != std::string::npos);
}

TEST_CASE("Back leaves a setting unchanged", "[config-menu][unit]") {
    Store store;
    std::istringstream input {"4\n0\n0\n"};
    std::ostringstream output;
    const auto result = menu::run_menu(
        "Slash Configuration",
        settings,
        [&](const menu::Setting& setting) { return current(store, setting); },
        [&](const menu::Setting& setting, const std::string_view value) {
            return apply(store, setting, value);
        },
        input,
        output
    );

    CHECK(result.ok);
    CHECK_FALSE(result.changed);
    CHECK(store.logging == "on");
}

TEST_CASE("A failed write stops the menu", "[config-menu][unit]") {
    Store store;
    store.fail = true;
    std::istringstream input {"4\n2\n"};
    std::ostringstream output;
    const auto result = menu::run_menu(
        "Slash Configuration",
        settings,
        [&](const menu::Setting& setting) { return current(store, setting); },
        [&](const menu::Setting& setting, const std::string_view value) {
            return apply(store, setting, value);
        },
        input,
        output
    );

    CHECK_FALSE(result.ok);
    CHECK(store.logging == "on");
}

TEST_CASE("Initialization offers the same configuration menu", "[config-menu][unit]") {
    Store store;
    std::istringstream invalid {"5\n0\n"};
    std::ostringstream output;
    CHECK(menu::offer_configuration(
        "Slash Configuration",
        settings,
        [&](const menu::Setting& setting) { return current(store, setting); },
        invalid,
        output
    ) == menu::OfferResult::leave);
    CHECK(output.str().find("Enter 0 or 1.\n") != std::string::npos);
    CHECK(output.str().find("1. Configure\n0. Continue\n") != std::string::npos);
    CHECK(output.str().find("Mode       verbose\n") != std::string::npos);

    std::istringstream configure {"1\n"};
    std::ostringstream configure_output;
    CHECK(menu::offer_configuration(
        "Slash Configuration",
        settings,
        [&](const menu::Setting& setting) { return current(store, setting); },
        configure,
        configure_output
    ) == menu::OfferResult::configure);
}
