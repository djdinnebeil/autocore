/**
 * \file journal_clock.ixx
 * \brief Provides journal-specific extended-day timestamps.
 */
export module journal_clock;

import std;

export namespace journal_clock {
    /** Returns the configured rollover hour from 0 through 12. */
    [[nodiscard]] int day_rollover_hour();

    /** Returns the current time using the journal's extended-day notation. */
    [[nodiscard]] std::string get_extended_timestamp();
}
