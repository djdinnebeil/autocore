/**
 * \file journal_clock.ixx
 * \brief Journal day-rollover timestamp from `journal.ini`.
 */
export module journal_clock;

import std;

export namespace journal_clock {
    /**
     * `[timestamp] day_rollover_hour` in `journal.ini`, clamped to 0–12.
     * Missing or invalid values become 0.
     */
    [[nodiscard]] int day_rollover_hour();
    /** `ac::clock::get_extended_timestamp` using `day_rollover_hour()`. */
    [[nodiscard]] std::string get_extended_timestamp();
}
