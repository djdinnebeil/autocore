module journal_clock;

import std;
import auto_core.core.clock;
import journal_extended_hours;

namespace journal_clock {

std::string get_extended_timestamp() {
    static const auto hours = journal::extended_hours::load();
    const auto stamp = ac::clock::get_timestamp();
    int hour = 0;
    int minute = 0;
    if (stamp.size() == 5 && stamp[2] == ':') {
        const auto hour_result = std::from_chars(
            stamp.data(),
            stamp.data() + 2,
            hour
        );
        const auto minute_result = std::from_chars(
            stamp.data() + 3,
            stamp.data() + 5,
            minute
        );
        if (hour_result.ec != std::errc {} ||
            hour_result.ptr != stamp.data() + 2 ||
            minute_result.ec != std::errc {} ||
            minute_result.ptr != stamp.data() + 5) {
            hour = 0;
            minute = 0;
        }
    }
    return journal::extended_hours::format(hours, hour, minute);
}

} // namespace journal_clock
