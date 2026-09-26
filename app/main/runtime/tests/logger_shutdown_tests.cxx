#include "catch_amalgamated.hpp"
#include "logger_shutdown_detail.hpp"

TEST_CASE("Logger shutdown uses a fresh full timeout", "[components-list][shutdown]") {
    using namespace std::chrono_literals;
    const auto configured = 2000ms;
    const auto phase_one_elapsed = 1500ms;
    const auto remaining = configured - phase_one_elapsed;
    const auto phase_two =
        ac::main::components::detail::logger_phase_timeout(configured);

    CHECK(phase_two == configured);
    CHECK(phase_two != remaining);
    CHECK(ac::main::components::detail::is_logger("logger"));
    CHECK_FALSE(ac::main::components::detail::is_logger("writer"));
    CHECK_FALSE(ac::main::components::detail::host_periodic_logger(0));
    CHECK(ac::main::components::detail::host_periodic_logger(1));
    CHECK(ac::main::components::detail::host_periodic_logger(60));
    CHECK(ac::main::components::detail::launch_shutdown_once(true));
    CHECK_FALSE(ac::main::components::detail::launch_shutdown_once(false));
}
