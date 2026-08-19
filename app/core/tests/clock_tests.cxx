#include "catch_amalgamated.hpp"
#include "../src/clock_formatting.hpp"

#include <array>
#include <regex>

import auto_core.clock;

namespace {

    ac::clock::detail::LocalTime sample_time() {
        return {
            .year = 2007,
            .month = 1,
            .day = 2,
            .day_of_week = 2,
            .hour = 3,
            .minute = 4,
            .second = 5
        };
    }

} // namespace

TEST_CASE("Clock timestamps use fixed-width 24-hour fields", "[clock][unit]") {
    auto time = sample_time();
    CHECK(ac::clock::detail::format_timestamp(time) == "03:04");
    CHECK(
        ac::clock::detail::format_timestamp_with_seconds(time) ==
        "03:04:05"
    );

    time.hour = 23;
    time.minute = 59;
    time.second = 59;
    CHECK(ac::clock::detail::format_timestamp(time) == "23:59");
    CHECK(
        ac::clock::detail::format_timestamp_with_seconds(time) ==
        "23:59:59"
    );
}

TEST_CASE("Extended timestamps use the configured rollover", "[clock][unit]") {
    auto time = sample_time();
    time.minute = 30;

    SECTION("zero uses calendar hours") {
        time.hour = 0;
        CHECK(
            ac::clock::detail::format_extended_timestamp(time, 0) ==
            "00:30"
        );
    }

    SECTION("hours before six continue the previous logical day") {
        time.hour = 0;
        CHECK(
            ac::clock::detail::format_extended_timestamp(time, 6) ==
            "24:30"
        );

        time.hour = 5;
        CHECK(
            ac::clock::detail::format_extended_timestamp(time, 6) ==
            "29:30"
        );
    }

    SECTION("the rollover hour starts the next logical day") {
        time.hour = 6;
        CHECK(
            ac::clock::detail::format_extended_timestamp(time, 6) ==
            "06:30"
        );
    }

    SECTION("afternoon and evening remain in 24-hour notation") {
        time.hour = 13;
        CHECK(
            ac::clock::detail::format_extended_timestamp(time, 6) ==
            "13:30"
        );

        time.hour = 21;
        CHECK(
            ac::clock::detail::format_extended_timestamp(time, 6) ==
            "21:30"
        );
    }

    SECTION("noon is the largest supported rollover") {
        time.hour = 11;
        CHECK(
            ac::clock::detail::format_extended_timestamp(time, 12) ==
            "35:30"
        );

        time.hour = 12;
        CHECK(
            ac::clock::detail::format_extended_timestamp(time, 12) ==
            "12:30"
        );
    }
}

TEST_CASE("Every supported rollover accepts every local hour", "[clock][unit]") {
    auto time = sample_time();
    time.minute = 0;

    for (int rollover = 0; rollover <= 12; ++rollover) {
        for (int hour = 0; hour <= 23; ++hour) {
            time.hour = hour;
            const int expected_hour = hour < rollover ? hour + 24 : hour;

            INFO("rollover=" << rollover << ", hour=" << hour);
            CHECK(
                ac::clock::detail::format_extended_timestamp(time, rollover) ==
                std::format("{:02}:00", expected_hour)
            );
        }
    }
}

TEST_CASE("Clock dates use their documented formats", "[clock][unit]") {
    auto time = sample_time();
    CHECK(ac::clock::detail::format_date_iso(time) == "2007-01-02");
    CHECK(ac::clock::detail::format_date_compact(time) == "1-2-07");

    time.year = 2026;
    time.month = 12;
    time.day = 31;
    CHECK(ac::clock::detail::format_date_iso(time) == "2026-12-31");
    CHECK(ac::clock::detail::format_date_compact(time) == "12-31-26");
}

TEST_CASE("Clock formats a complete local datetime", "[clock][unit]") {
    const ac::clock::DateTime datetime {
        .date_iso = "2007-01-02",
        .timestamp = "03:04",
        .timestamp_with_seconds = "03:04:05"
    };

    CHECK(
        ac::clock::format_datetime(datetime) ==
        "2007-01-02 at 03:04:05"
    );
}

TEST_CASE("Clock exposes every weekday name", "[clock][unit]") {
    constexpr std::array<std::string_view, 7> expected {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    };
    auto time = sample_time();

    for (int day = 0; day < static_cast<int>(expected.size()); ++day) {
        time.day_of_week = day;
        CHECK(ac::clock::detail::format_day_of_week(time) == expected[day]);
    }
}

TEST_CASE("Internal clock formatters reject invalid fields", "[clock][unit]") {
    auto time = sample_time();

    CHECK_THROWS_AS(
        ac::clock::detail::format_extended_timestamp(time, -1),
        std::out_of_range
    );
    CHECK_THROWS_AS(
        ac::clock::detail::format_extended_timestamp(time, 13),
        std::out_of_range
    );

    time.hour = 24;
    CHECK_THROWS_AS(
        ac::clock::detail::format_timestamp(time),
        std::out_of_range
    );

    time = sample_time();
    time.minute = 60;
    CHECK_THROWS_AS(
        ac::clock::detail::format_timestamp(time),
        std::out_of_range
    );

    time = sample_time();
    time.day_of_week = 7;
    CHECK_THROWS_AS(
        ac::clock::detail::format_day_of_week(time),
        std::out_of_range
    );
}

TEST_CASE("Public clock functions return valid local-time shapes", "[clock][integration]") {
    const std::regex timestamp_pattern {R"(^\d{2}:\d{2}$)"};
    const std::regex timestamp_seconds_pattern {R"(^\d{2}:\d{2}:\d{2}$)"};
    const std::regex date_iso_pattern {R"(^\d{4}-\d{2}-\d{2}$)"};
    const std::regex date_compact_pattern {R"(^\d{1,2}-\d{1,2}-\d{2}$)"};
    const std::regex datetime_pattern {
        R"(^\d{4}-\d{2}-\d{2} at \d{2}:\d{2}:\d{2}$)"
    };

    const ac::clock::DateTime datetime = ac::clock::get_local_datetime();
    CHECK(std::regex_match(datetime.date_iso, date_iso_pattern));
    CHECK(std::regex_match(datetime.timestamp, timestamp_pattern));
    CHECK(std::regex_match(
        datetime.timestamp_with_seconds,
        timestamp_seconds_pattern
    ));
    CHECK(
        datetime.timestamp_with_seconds.substr(0, 5) ==
        datetime.timestamp
    );

    CHECK(std::regex_match(ac::clock::get_timestamp(), timestamp_pattern));
    CHECK(std::regex_match(
        ac::clock::get_timestamp_with_seconds(),
        timestamp_seconds_pattern
    ));
    CHECK(std::regex_match(
        ac::clock::get_extended_timestamp(6),
        timestamp_pattern
    ));
    CHECK(std::regex_match(ac::clock::get_date_iso(), date_iso_pattern));
    CHECK(std::regex_match(ac::clock::get_datetime(), datetime_pattern));
    CHECK(std::regex_match(
        ac::clock::get_date_compact(),
        date_compact_pattern
    ));

    const std::array<std::string_view, 7> weekdays {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    };
    CHECK(std::ranges::find(weekdays, ac::clock::get_day_of_week()) !=
        weekdays.end());

}
