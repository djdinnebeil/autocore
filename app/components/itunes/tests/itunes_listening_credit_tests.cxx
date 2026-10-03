#include "catch_amalgamated.hpp"
#include "../shared/listening_credit.hpp"

namespace listening = itunes::listening;

listening::CreditInput credit_input(
    const listening::WakeKind wake,
    const bool playing,
    const int position_seconds,
    const int scheduled_sleep_seconds = 5
) {
    return {
        .wake = wake,
        .scheduled_sleep_seconds = scheduled_sleep_seconds,
        .playing = playing,
        .position_seconds = position_seconds,
        .idle_interval_seconds = 5,
    };
}

TEST_CASE("Initial and special wakes credit nothing", "[itunes][listening][unit]") {
    CHECK(listening::credit_seconds(credit_input(listening::WakeKind::initial, true, 10)) == 0);
    CHECK(listening::credit_seconds(credit_input(listening::WakeKind::special, true, 10)) == 0);
}

TEST_CASE("A playing timeout credits the scheduled sleep", "[itunes][listening][unit]") {
    CHECK(listening::credit_seconds(credit_input(listening::WakeKind::playing_timeout, true, 30, 5)) == 5);
    CHECK(listening::credit_seconds(credit_input(listening::WakeKind::playing_timeout, true, 30, 2)) == 2);
    CHECK(listening::credit_seconds(credit_input(listening::WakeKind::playing_timeout, false, 30, 5)) == 0);
}

TEST_CASE("An idle timeout credits position only below the idle interval", "[itunes][listening][unit]") {
    CHECK(listening::credit_seconds(credit_input(listening::WakeKind::idle_timeout, true, 0)) == 0);
    CHECK(listening::credit_seconds(credit_input(listening::WakeKind::idle_timeout, true, 4)) == 4);
    CHECK(listening::credit_seconds(credit_input(listening::WakeKind::idle_timeout, true, 5)) == 5);
    CHECK(listening::credit_seconds(credit_input(listening::WakeKind::idle_timeout, true, 47)) == 5);
    CHECK(listening::credit_seconds(credit_input(listening::WakeKind::idle_timeout, true, -1)) == 0);
    CHECK(listening::credit_seconds(credit_input(listening::WakeKind::idle_timeout, false, 4)) == 0);
}
