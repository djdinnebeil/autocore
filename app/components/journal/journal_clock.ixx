export module journal_clock;

import std;

export namespace journal_clock {
    [[nodiscard]] int day_rollover_hour();
    [[nodiscard]] std::string get_extended_timestamp();
}
