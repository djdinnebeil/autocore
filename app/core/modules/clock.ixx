/**
 * \file clock.ixx
 * \brief Provides local date and time values in standard and custom formats.
 *
 * `get_local_datetime` and `get_datetime` share one clock snapshot.
 * Every other getter reads the local clock independently.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.clock;

import std;

export namespace ac::clock {

    /**
     * \brief Local date and time strings derived from one clock snapshot.
     *
     * `date_iso` uses `YYYY-MM-DD`, `timestamp` uses `HH:MM`, and
     * `timestamp_with_seconds` uses `HH:MM:SS`.
     */
    struct DateTime {
        std::string date_iso;
        std::string timestamp;
        std::string timestamp_with_seconds;
    };

    /**
     * \brief Returns the current local date and time from one clock snapshot.
     */
    [[nodiscard]]
    AC_API DateTime get_local_datetime();

    /**
     * \brief Formats a local date and time as `YYYY-MM-DD at HH:MM:SS`.
     */
    [[nodiscard]]
    AC_API std::string format_datetime(const DateTime& datetime);

    /**
     * \brief Returns the current local date and time as
     * `YYYY-MM-DD at HH:MM:SS`.
     *
     * The date and time are derived from one clock snapshot.
     */
    [[nodiscard]]
    AC_API std::string get_datetime();

    /**
     * \brief Returns the current local time in 24-hour `HH:MM` format.
     */
    [[nodiscard]]
    AC_API std::string get_timestamp();

    /**
     * \brief Returns the current local time in 24-hour `HH:MM:SS` format.
     */
    [[nodiscard]]
    AC_API std::string get_timestamp_with_seconds();

    /**
     * \brief Returns the current local time using extended-day notation.
     *
     * Hours before `day_rollover_hour` have 24 added. At and after that hour,
     * ordinary 24-hour notation is used. `day_rollover_hour` must be between
     * 0 and 12 inclusive.
     *
     * For example, with a rollover hour of 4, `01:30` is returned as `25:30`,
     * while `04:30` remains `04:30`.
     *
     * This function does not adjust the calendar date. It reads the local
     * clock independently of `get_local_datetime`.
     *
     * \param day_rollover_hour The first hour of the next logical day.
     * \return The formatted extended-day timestamp.
     * \throws std::out_of_range if `day_rollover_hour` is outside 0 through 12.
     */
    [[nodiscard]]
    AC_API std::string get_extended_timestamp(int day_rollover_hour);

    /**
     * \brief Returns the current local calendar date as `YYYY-MM-DD`.
     */
    [[nodiscard]]
    AC_API std::string get_date_iso();

    /**
     * \brief Returns the current local calendar date as `M-D-YY`.
     *
     * Month and day are not padded; the two-digit year is zero-padded.
     */
    [[nodiscard]]
    AC_API std::string get_date_compact();

    /**
     * \brief Returns the current local weekday as a full English name.
     */
    [[nodiscard]]
    AC_API std::string get_day_of_week();
} // namespace ac::clock
