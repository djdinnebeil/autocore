/**
 * \file clock.ixx
 * \brief Provides local date and time retrieval and formatting.
 */
module;

#include "ac_api.hpp"

export module clock;

import std;

export namespace ac::clock {
    struct DateTime {
        std::string date_iso;
        std::string timestamp;
        std::string timestamp_with_seconds;
    };

    AC_API DateTime get_local_datetime();

    AC_API std::string get_timestamp();
    AC_API std::string get_timestamp_with_seconds();
    AC_API std::string get_extended_timestamp();

    AC_API std::string get_date_iso();
    AC_API std::string get_date_compact();
    
    AC_API std::string get_day_of_week();
}