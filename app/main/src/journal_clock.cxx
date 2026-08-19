module journal_clock;

import std;
import auto_core.clock;
import auto_core.ini;
import auto_core.paths;

namespace journal_clock {
    int day_rollover_hour() {
        static const int hour = [] {
            const auto document = ac::ini::read(
                ac::paths::config_directory() / "journal.ini"
            );
            if (!document) {
                return 0;
            }

            const auto value =
                document->find("journal", "day_rollover_hour");
            if (!value) {
                return 0;
            }

            int parsed = 0;
            const auto result = std::from_chars(
                value->data(), value->data() + value->size(), parsed
            );
            return result.ec == std::errc {} &&
                result.ptr == value->data() + value->size() &&
                parsed >= 0 && parsed <= 12
                ? parsed
                : 0;
        }();

        return hour;
    }

    std::string get_extended_timestamp() {
        return ac::clock::get_extended_timestamp(day_rollover_hour());
    }
}
