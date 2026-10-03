#include "catch_amalgamated.hpp"

#include <sstream>
#include <string>

import slash_defaults;
import slash_config_detail;

using slash::config::Action;

TEST_CASE("fresh Slash initialization accepts compiled defaults", "[slash][config]") {
    std::istringstream input {"\n\n"};
    std::ostringstream output;

    REQUIRE(slash::config::select_action(false, true, false) == Action::initialize);
    const auto mode = slash::config::prompt_mode(
        input,
        output,
        slash::defaults::mode_verbose
    );
    const auto logging = slash::config::prompt_logging(input, output, true);

    REQUIRE(mode);
    REQUIRE(logging);
    CHECK(*mode == "verbose");
    CHECK(*logging);
    CHECK(output.str() ==
        "Slash mode [verbose]: Enable logging [on]: ");
    CHECK(slash::defaults::ini_for_mode(*mode, *logging) ==
        slash::defaults::ini_text);
}

TEST_CASE("Slash mode accepts only its three configured values", "[slash][config]") {
    for (const std::string value : {"verbose", "concise", "silent"}) {
        std::istringstream input {value + "\n"};
        std::ostringstream output;
        CHECK(slash::config::prompt_mode(input, output, "verbose") == value);
    }

    std::istringstream input {"detailed\nconcise\n"};
    std::ostringstream output;
    CHECK(slash::config::prompt_mode(input, output, "verbose") == "concise");
    CHECK(output.str() ==
        "Slash mode [verbose]: Enter verbose, concise, or silent.\n"
        "Slash mode [verbose]: ");
}

TEST_CASE("Slash logging accepts on off and the bracketed value", "[slash][config]") {
    {
        std::istringstream input {"on\n"};
        std::ostringstream output;
        CHECK(slash::config::prompt_logging(input, output, false) == true);
        CHECK(output.str() == "Enable logging [off]: ");
    }
    {
        std::istringstream input {"off\n"};
        std::ostringstream output;
        CHECK(slash::config::prompt_logging(input, output, true) == false);
        CHECK(output.str() == "Enable logging [on]: ");
    }
    {
        std::istringstream input {"\n"};
        std::ostringstream output;
        CHECK(slash::config::prompt_logging(input, output, false) == false);
    }
    {
        std::istringstream input {"yes\noff\n"};
        std::ostringstream output;
        CHECK(slash::config::prompt_logging(input, output, true) == false);
        CHECK(output.str() ==
            "Enable logging [on]: Enter on or off.\n"
            "Enable logging [on]: ");
    }
}

TEST_CASE("Slash seed is noninteractive and has precedence", "[slash][config]") {
    CHECK(slash::config::select_action(false, false, true) == Action::seed);
    CHECK(slash::config::select_action(false, true, true) == Action::seed);
    CHECK(std::string {slash::defaults::ini_text} ==
        "[slash]\n"
        "mode = verbose\n"
        "logging = on\n");
}

TEST_CASE("existing Slash initialization state is preserved", "[slash][config]") {
    CHECK(slash::config::select_action(true, true, false) ==
        Action::skip_initialization);
    CHECK(slash::config::select_action(true, false, true) == Action::skip_seed);
    CHECK(slash::config::select_action(true, true, true) == Action::skip_seed);
}

TEST_CASE("Slash Configure shows and changes current values", "[slash][config]") {
    REQUIRE(slash::config::select_action(true, false, false) == Action::configure);
    std::istringstream input {"silent\non\n"};
    std::ostringstream output;

    const auto mode = slash::config::prompt_mode(input, output, "concise");
    const auto logging = slash::config::prompt_logging(input, output, false);

    REQUIRE(mode);
    REQUIRE(logging);
    CHECK(*mode == "silent");
    CHECK(*logging);
    CHECK(output.str() ==
        "Slash mode [concise]: Enable logging [off]: ");
    CHECK(slash::defaults::ini_for_mode(*mode, *logging) ==
        "[slash]\nmode = silent\nlogging = on\n");
}
