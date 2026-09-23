module journal_clock;

import std;
import auto_core.core.clock;
import auto_core.core.ini;
import auto_core.core.paths;
import journal_defaults;

namespace journal_clock {

int day_rollover_hour() {
    static const int hour = [] {
        const auto document = ac::ini::read(
            ac::paths::config_directory() / "journal.ini"
        );
        if (!document) {
            return journal::defaults::day_rollover_hour;
        }

        const auto value = document->find("timestamp", "day_rollover_hour");
        if (!value) {
            return journal::defaults::day_rollover_hour;
        }

        int parsed = 0;
        const auto result = std::from_chars(
            value->data(), value->data() + value->size(), parsed
        );
        return result.ec == std::errc {} &&
            result.ptr == value->data() + value->size() &&
            parsed >= 0 && parsed <= 12
            ? parsed
            : journal::defaults::day_rollover_hour;
    }();

    return hour;
}

std::string get_extended_timestamp() {
    return ac::clock::get_extended_timestamp(day_rollover_hour());
}

} // namespace journal_clock
