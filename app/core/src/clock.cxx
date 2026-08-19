/**
 * \file clock.cxx
 * \brief Implements local system-clock retrieval for the clock module.
 */
module;

#include "clock_formatting.hpp"

module auto_core.clock;

import <Windows.h>;

namespace {

    ac::clock::detail::LocalTime get_local_time() noexcept {
        SYSTEMTIME time {};
        GetLocalTime(&time);

        return {
            .year = time.wYear,
            .month = time.wMonth,
            .day = time.wDay,
            .day_of_week = time.wDayOfWeek,
            .hour = time.wHour,
            .minute = time.wMinute,
            .second = time.wSecond
        };
    }

} // namespace

namespace ac::clock {

    DateTime get_local_datetime() {
        const detail::LocalTime time = get_local_time();

        return {
            .date_iso = detail::format_date_iso(time),
            .timestamp = detail::format_timestamp(time),
            .timestamp_with_seconds =
                detail::format_timestamp_with_seconds(time)
        };
    }

    std::string format_datetime(const DateTime& datetime) {
        return std::format(
            "{} at {}",
            datetime.date_iso,
            datetime.timestamp_with_seconds
        );
    }

    std::string get_datetime() {
        return format_datetime(get_local_datetime());
    }

    std::string get_timestamp() {
        return detail::format_timestamp(get_local_time());
    }

    std::string get_timestamp_with_seconds() {
        return detail::format_timestamp_with_seconds(get_local_time());
    }

    std::string get_extended_timestamp(const int day_rollover_hour) {
        return detail::format_extended_timestamp(
            get_local_time(),
            day_rollover_hour
        );
    }

    std::string get_date_iso() {
        return detail::format_date_iso(get_local_time());
    }

    std::string get_date_compact() {
        return detail::format_date_compact(get_local_time());
    }

    std::string get_day_of_week() {
        return detail::format_day_of_week(get_local_time());
    }

} // namespace ac::clock
