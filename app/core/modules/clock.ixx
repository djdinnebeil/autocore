/**
 * \file clock.ixx
 * \brief Offers clock and timing functionalities, including timestamping and timers.
 */
module;

#ifdef BUILDING_DLL
    #define DLL_API __declspec(dllexport)
#else
    #define DLL_API __declspec(dllimport)
#endif

export module clock;
import std;
import config;
import <Windows.h>;

export namespace ac::clock {
    DLL_API int get_minutes_stamp();
    DLL_API int get_current_seconds();
    DLL_API std::string get_timestamp();
    DLL_API std::string get_datestamp();
    DLL_API std::string get_datestamp_iso();
    DLL_API std::string get_datestamp_iso_with_timestamp();
    DLL_API std::string get_timestamp_with_seconds();
    DLL_API std::string get_datetime_stamp_with_seconds();
    DLL_API std::string get_datetime_stamp_for_logger();
    DLL_API std::string get_day_of_week();
}
