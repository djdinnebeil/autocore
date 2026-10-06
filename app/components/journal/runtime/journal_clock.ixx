/**
 * \file journal_clock.ixx
 * \brief Journal extended timestamp from `extended_hour.clock`.
 */
export module journal_clock;

import std;

export namespace journal_clock {
    /**
     * Current local time under the extended-hour token in
     * `extended_hour.clock` in the journal data directory.
     *
     * A missing or malformed file is token `0`. This function does not
     * write the file.
     */
    [[nodiscard]] std::string get_extended_timestamp();
}
