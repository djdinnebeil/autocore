/**
 * \file clock_formatting.cxx
 * \brief Implements deterministic local date and time formatting.
 */
#include "clock_formatting.hpp"

#include <array>
#include <format>
#include <stdexcept>
#include <string_view>

namespace {

    void require_range(
        const int value,
        const int minimum,
        const int maximum,
        const std::string_view name
    ) {
        if (value < minimum || value > maximum) {
            throw std::out_of_range(std::format(
                "Invalid {}: {}. Expected {} through {}.",
                name,
                value,
                minimum,
                maximum
            ));
        }
    }

    void validate_time(const ac::clock::detail::LocalTime& time) {
        require_range(time.hour, 0, 23, "hour");
        require_range(time.minute, 0, 59, "minute");
        require_range(time.second, 0, 59, "second");
    }

    void validate_date(const ac::clock::detail::LocalTime& time) {
        require_range(time.year, 0, 9999, "year");
        require_range(time.month, 1, 12, "month");
        require_range(time.day, 1, 31, "day");
    }

} // namespace

namespace ac::clock::detail {

    std::string format_timestamp(const LocalTime& time) {
        validate_time(time);
        return std::format("{:02}:{:02}", time.hour, time.minute);
    }

    std::string format_timestamp_with_seconds(const LocalTime& time) {
        validate_time(time);
        return std::format(
            "{:02}:{:02}:{:02}",
            time.hour,
            time.minute,
            time.second
        );
    }

    std::string format_extended_timestamp(
        const LocalTime& time,
        const int day_rollover_hour
    ) {
        validate_time(time);
        require_range(
            day_rollover_hour,
            0,
            12,
            "next-day rollover hour"
        );

        const int extended_hour = time.hour < day_rollover_hour
            ? time.hour + 24
            : time.hour;

        return std::format("{:02}:{:02}", extended_hour, time.minute);
    }

    std::string format_date_iso(const LocalTime& time) {
        validate_date(time);
        return std::format(
            "{:04}-{:02}-{:02}",
            time.year,
            time.month,
            time.day
        );
    }

    std::string format_date_compact(const LocalTime& time) {
        validate_date(time);
        return std::format(
            "{}-{}-{:02}",
            time.month,
            time.day,
            time.year % 100
        );
    }

    std::string format_day_of_week(const LocalTime& time) {
        constexpr std::array<std::string_view, 7> days {
            "Sunday", "Monday", "Tuesday", "Wednesday",
            "Thursday", "Friday", "Saturday"
        };

        require_range(time.day_of_week, 0, 6, "day of week");
        return std::string {days[time.day_of_week]};
    }

} // namespace ac::clock::detail
