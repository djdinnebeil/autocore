/**
 * \file listening_credit.hpp
 * \brief Approximate listening credit for one existing iTunes monitor wake.
 *
 * The monitor remains the timing source. This function does not detect seeks,
 * pauses, or repeat mode.
 */
#pragma once

namespace itunes::listening {

enum class WakeKind {
    initial,
    playing_timeout,
    idle_timeout,
    special
};

struct CreditInput {
    WakeKind wake = WakeKind::initial;
    int scheduled_sleep_seconds = 0;
    bool playing = false;
    int position_seconds = 0;
    int idle_interval_seconds = 5;
};

inline constexpr int conservative_credit_seconds = 5;

[[nodiscard]] inline int credit_seconds(const CreditInput& input) noexcept {
    if (input.wake == WakeKind::initial ||
        input.wake == WakeKind::special ||
        !input.playing) {
        return 0;
    }
    if (input.wake == WakeKind::playing_timeout) {
        return input.scheduled_sleep_seconds > 0 ? input.scheduled_sleep_seconds : 0;
    }
    if (input.position_seconds < 0) {
        return 0;
    }
    if (input.position_seconds < input.idle_interval_seconds) {
        return input.position_seconds;
    }
    return conservative_credit_seconds;
}

} // namespace itunes::listening
