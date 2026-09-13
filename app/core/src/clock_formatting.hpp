/**
 * \file clock_formatting.hpp
 * \brief Provides deterministic formatting for the clock module.
 *
 * These internal helpers operate on supplied local date and time fields
 * rather than reading the system clock.
 */
#pragma once

#include <string>

namespace ac::clock::detail {

    /**
     * \brief Calendar and clock fields used by internal formatters.
     *
     * Fields are validated individually by the formatter that uses them;
     * calendar relationships such as month-specific day counts are not
     * validated.
     *
     * `day_of_week` follows the Windows `SYSTEMTIME` convention: Sunday is 0
     * and Saturday is 6.
     */
    struct LocalTime {
        int year;
        int month;
        int day;
        int day_of_week;
        int hour;
        int minute;
        int second;
    };

    /**
     * \brief Formats local time as `HH:MM`.
     *
     * \param time The local date and time fields to format.
     * \return The formatted 24-hour timestamp.
     * \throws std::out_of_range if a clock field is outside its valid range.
     */
    std::string format_timestamp(const LocalTime& time);

    /**
     * \brief Formats local time as `HH:MM:SS`.
     *
     * \param time The local date and time fields to format.
     * \return The formatted 24-hour timestamp including seconds.
     * \throws std::out_of_range if a clock field is outside its valid range.
     */
    std::string format_timestamp_with_seconds(const LocalTime& time);

    /**
     * \brief Formats local time using next-day rollover notation.
     *
     * Hours before `day_rollover_hour` have 24 added. The rollover hour
     * must be between 0 and 12 inclusive.
     *
     * \param time The local date and time fields to format.
     * \param day_rollover_hour The first hour of the next logical day.
     * \return The formatted extended-day timestamp.
     * \throws std::out_of_range if a clock field or rollover hour is outside
     * its valid range.
     */
    std::string format_extended_timestamp(
        const LocalTime& time,
        int day_rollover_hour
    );

    /**
     * \brief Formats a local calendar date as `YYYY-MM-DD`.
     *
     * \param time The local date and time fields to format.
     * \return The zero-padded ISO calendar date.
     * \throws std::out_of_range if a date field is outside its valid range.
     */
    std::string format_date_iso(const LocalTime& time);

    /**
     * \brief Formats a local calendar date as `M-D-YY`.
     *
     * Month and day are unpadded; the two-digit year is zero-padded.
     *
     * \param time The local date and time fields to format.
     * \return The compact calendar date.
     * \throws std::out_of_range if a date field is outside its valid range.
     */
    std::string format_date_compact(const LocalTime& time);

    /**
     * \brief Returns the full English name of the local weekday.
     *
     * \param time The local date and time fields containing the weekday.
     * \return The full English weekday name, from Sunday through Saturday.
     * \throws std::out_of_range if `day_of_week` is outside 0 through 6.
     */
    std::string format_day_of_week(const LocalTime& time);

} // namespace ac::clock::detail
