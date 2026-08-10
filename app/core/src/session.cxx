module session;

import clock;
import std;

namespace ac::session {
    Start make_start() {
        const auto datetime = ac::clock::get_local_datetime();

        return {
            .date_iso = datetime.date_iso,
            .timestamp_with_seconds =
                datetime.timestamp_with_seconds,
            .datetime = std::format(
                "{} at {}",
                datetime.date_iso,
                datetime.timestamp_with_seconds
            )
        };
    }

    std::string make_datetime() {
        const auto datetime = ac::clock::get_local_datetime();

        return std::format(
            "{} at {}",
            datetime.date_iso,
            datetime.timestamp_with_seconds
        );
    }

}