/**
\file logger.ixx
\brief Defines a custom logger featuring logg() and loggnl().

This module provides logging functionality for the Auto Core application. It includes
functions to log messages with and without newline characters, update the log file,
and handle formatting errors.
*/
module;

#ifdef BUILDING_DLL
    #define DLL_API __declspec(dllexport)
#else
    #define DLL_API __declspec(dllimport)
#endif

export module logger;
import std;
import config;
import encoding;
import clock;
import <Windows.h>;

export std::string session_start;
export std::ofstream main_log_stream;
export std::string main_log_name;
export std::string logger_datestamp;
export std::string log_directory = ac::config.logger_directory;

export namespace ac::logger {
    DLL_API void logg(const std::string& msg);
    DLL_API void loggnl(const std::string& msg);
    DLL_API void logg(const std::wstring& msg);
    DLL_API void loggnl(const std::wstring& msg);
    DLL_API void logg(char msg);
    DLL_API void loggnl(char msg);
    DLL_API void logg(wchar_t msg);
    DLL_API void loggnl(wchar_t msg);
    DLL_API void update_main_log_file();
    DLL_API void close_main_log_file();
    DLL_API void log_end();
    template<typename... Args> void logg(const char* format_string, Args&&... args);
    template<typename... Args> void loggnl(const char* format_string, Args&&... args);
}

namespace ac::logger {
    using logger_func = void(*)(const std::string&);

    template<typename... Args>
    void logger_template(const char* format_string, logger_func logger, Args&&... args) {
        try {
            if (format_string == nullptr) {
                throw std::invalid_argument("Format std::string is null");
            }
            auto convert_arg = [](auto&& arg) {
                if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, const wchar_t*>) {
                    return ac::encoding::to_utf8(arg);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::wstring>) {
                    return ac::encoding::to_utf8(arg);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, wchar_t>) {
                    return ac::encoding::to_utf8(arg);
                }
                else {
                    return std::forward<decltype(arg)>(arg);
                }
                };

            auto tuple_args = std::make_tuple(convert_arg(args)...);
            auto formatted_string = std::apply([&](auto&&... unpacked_args) {
                return std::vformat(format_string, std::make_format_args(unpacked_args...));
                }, tuple_args);

            logger(formatted_string);
        }
        catch (const std::format_error& e) {
            std::string error = std::format("Format error: {}", e.what());
            logg(error);
        }
        catch (const std::invalid_argument& e) {
            std::string error = std::format("Invalid argument: {}", e.what());
            logg(error);
        }
    }
}

template<typename... Args>
void ac::logger::logg(const char* format_string, Args&&... args) {
    ac::logger::logger_template(format_string, logg, std::forward<Args>(args)...);
}

template<typename... Args>
void ac::logger::loggnl(const char* format_string, Args&&... args) {
    ac::logger::logger_template(format_string, loggnl, std::forward<Args>(args)...);
}
