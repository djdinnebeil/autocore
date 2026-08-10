module clock;

import std;
import config;
import <Windows.h>;

namespace ac::clock {

    /**
     * \brief Retrieves the local system time.
     * \return The current local system time as SYSTEMTIME.
     */
    static SYSTEMTIME get_local_time() noexcept {
        SYSTEMTIME st {};
        GetLocalTime(&st);
        return st;
    }

    /**
     * \brief Formats a local-system hour in 24-hour format.
     * \param hour The hour from 0 through 23.
     * \return The formatted hour in "HH" format.
     */
    static std::string format_hour(const int hour) {
        return std::format("{:02}", hour);
    }

    /**
     * \brief Formats an hour using the configured extended-day boundary.
     * \param hour The local-system hour from 0 through 23.
     * \return The formatted extended hour.
     */
    static std::string format_extended_hour(const int hour) {
        if (hour < 0 || hour > 23) {
            throw std::out_of_range(
                std::format("Invalid hour: {}. Expected 0 through 23.", hour)
            );
        }

        if (hour < config::end_of_day()) {
            return std::format("{}", hour + 24);
        }

        if (hour >= 13 && hour < 22) {
            return std::format("{}", hour - 12);
        }

        if (hour < 10) {
            return std::format("{:02}", hour);
        }

        return std::format("{}", hour);
    }

    /**
     * \brief Formats the minutes value and converts it to a std::string.
     * \param minutes The minutes value to format.
     * \return The formatted minutes as a std::string.
     */
    static std::string format_minutes(int minutes) {
        return std::format("{:02}", minutes);
    }

    /**
     * \brief Formats the seconds value and converts it to a std::string.
     * \param seconds The seconds value to format.
     * \return The formatted seconds as a std::string.
     */
    static std::string format_seconds(int seconds) {
        return std::format("{:02}", seconds);
    }

    static std::string format_timestamp(const SYSTEMTIME& st) {
        std::ostringstream timestamp;
        timestamp << format_hour(st.wHour) << ':'
            << format_minutes(st.wMinute);

        return timestamp.str();
    }

    static std::string format_extended_timestamp(
        const SYSTEMTIME& st
    ) {
        std::ostringstream timestamp;
        timestamp << format_extended_hour(st.wHour) << ':'
            << format_minutes(st.wMinute);

        return timestamp.str();
    }

    static std::string format_timestamp_with_seconds(const SYSTEMTIME& st) {
        std::ostringstream timestamp;
        timestamp << format_hour(st.wHour) << ':'
            << format_minutes(st.wMinute) << ':'
            << format_seconds(st.wSecond);

        return timestamp.str();
    }

    static std::string format_date_iso(const SYSTEMTIME& st) {
        std::ostringstream date;
        date << std::setfill('0')
            << std::setw(4) << st.wYear << '-'
            << std::setw(2) << st.wMonth << '-'
            << std::setw(2) << st.wDay;

        return date.str();
    }

    /**
     * \brief Retrieves the current local date and timestamp.
     *
     * Both values are generated from the same local system-time snapshot.
     *
     * \return The current ISO date, timestamp without seconds, and timestamp with seconds.
     */
    DateTime get_local_datetime() {
        const SYSTEMTIME st = get_local_time();

        return {
            .date_iso = format_date_iso(st),
            .timestamp = format_timestamp(st),
            .timestamp_with_seconds =
                format_timestamp_with_seconds(st)
        };
    }
    /**
     * \brief Retrieves the current timestamp in "HH:MM" format.
     * \return The current timestamp as a std::string.
     */
    std::string get_timestamp() {
        const SYSTEMTIME st = get_local_time();
        return format_timestamp(st);
    }

    /**
     * \brief Retrieves the current timestamp with seconds in "HH:MM:SS" format.
     * \return The current timestamp with seconds as a std::string.
     */
    std::string get_timestamp_with_seconds() {
        const SYSTEMTIME st = get_local_time();
        return format_timestamp_with_seconds(st);
    }

    /**
     * \brief Retrieves the current extended timestamp in "HH:MM" format.
     *
     * Hours before the configured end of day are represented as hours
     * beyond 23, such as "24:30".
     *
     * \return The current extended timestamp.
     */
    std::string get_extended_timestamp() {
        const SYSTEMTIME st = get_local_time();
        return format_extended_timestamp(st);
    }

    /**
     * \brief Retrieves the current local date in compact format.
     * \return The current date in "M-D-YY" format.
     */
    std::string get_date_compact() {
        const SYSTEMTIME st = get_local_time();

        std::ostringstream date;
        date << st.wMonth << '-'
            << st.wDay << '-'
            << st.wYear % 100;

        return date.str();
    }

    /**
     * \brief Retrieves the current local date in ISO format.
     * \return The current date in "YYYY-MM-DD" format.
     */
    std::string get_date_iso() {
        const SYSTEMTIME st = get_local_time();
        return format_date_iso(st);
    }

    /**
     * \brief Retrieves the current day of the week.
     * \return The current day of the week as a std::string.
     */
    std::string get_day_of_week() {
        const SYSTEMTIME st = get_local_time();

        const char* days_of_week[7] = {
            "Sunday",
            "Monday",
            "Tuesday",
            "Wednesday",
            "Thursday",
            "Friday",
            "Saturday"
        };

        return days_of_week[st.wDayOfWeek];
    }
}