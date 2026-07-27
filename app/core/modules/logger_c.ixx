 /**
 \file logger_c.ixx
 \brief Offers specialized logging capabilities focused on individual components like music player activity.

 This module provides a Logger class that supports specialized logging for individual components.
 It includes functions for logging messages, updating log files, and handling formatted log messages.
  */
module;

#ifdef BUILDING_DLL
    #define DLL_API __declspec(dllexport)
#else
    #define DLL_API __declspec(dllimport)
#endif

export module logger_c;
import std;
import config;
import encoding;
import clock;
import logger;
import print;
import <Windows.h>;

export namespace ac {
    class Logger {
    public:
        DLL_API Logger(const std::string& log_name);
        DLL_API ~Logger();
        std::string name;
        std::string directory;
        std::string session_started;
        DLL_API void logg(const std::string& msg);
        DLL_API void loggnl(const std::string& msg);
        DLL_API void logg_and_print(const std::string& msg);
        DLL_API void logg_and_logg(const std::string& msg);
        DLL_API void loggnl_and_printnl(const std::string& msg);
        DLL_API void loggnl_and_loggnl(const std::string& msg);
        DLL_API void logg(const std::wstring& msg);
        DLL_API void loggnl(const std::wstring& msg);
        DLL_API void logg_and_print(const std::wstring& msg);
        DLL_API void logg_and_logg(const std::wstring& msg);
        DLL_API void loggnl_and_printnl(const std::wstring& msg);
        DLL_API void loggnl_and_loggnl(const std::wstring& msg);
        DLL_API void logg(char msg);
        DLL_API void loggnl(char msg);
        DLL_API void logg_and_print(char msg);
        DLL_API void logg_and_logg(char msg);
        DLL_API void loggnl_and_printnl(char msg);
        DLL_API void loggnl_and_loggnl(char msg);
        DLL_API void logg(wchar_t msg);
        DLL_API void loggnl(wchar_t msg);
        DLL_API void logg_and_print(wchar_t msg);
        DLL_API void logg_and_logg(wchar_t msg);
        DLL_API void loggnl_and_printnl(wchar_t msg);
        DLL_API void loggnl_and_loggnl(wchar_t msg);
        template<typename... Args>
        void logg(const char* format_string, Args&&... args);
        template<typename... Args>
        void loggnl(const char* format_string, Args&&... args);
        template<typename... Args>
        void logg_and_print(const char* format_string, Args&&... args);
        template<typename... Args>
        void loggnl_and_printnl(const char* format_string, Args&&... args);
        template<typename... Args>
        void logg_and_logg(const char* format_string, Args&&... args);
        template<typename... Args>
        void loggnl_and_loggnl(const char* format_string, Args&&... args);
        DLL_API void update_log_file();
        DLL_API void open_log_file();
        DLL_API void close_log_file();
        std::ofstream log_stream;
    };
}

template<typename... Args>
void ac::Logger::logg(const char* format_string, Args&&... args) {
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
        log_stream << formatted_string << std::endl;
        if (config.send_logg_to_cout) {
            std::cout << formatted_string << std::endl;
        }
    }
    catch (const std::format_error& e) {
        std::string error = std::format("Format error: {}", e.what());
        ac::logger::logg(error);
    }
    catch (const std::invalid_argument& e) {
        std::string error = std::format("Invalid argument: {}", e.what());
        ac::logger::logg(error);
    }
}

template<typename... Args>
void ac::Logger::loggnl(const char* format_string, Args&&... args) {
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
        log_stream << formatted_string;
        if (config.send_logg_to_cout) {
            std::cout << formatted_string;
        }
    }
    catch (const std::format_error& e) {
        std::string error = std::format("Format error: {}", e.what());
        ac::logger::logg(error);
    }
    catch (const std::invalid_argument& e) {
        std::string error = std::format("Invalid argument: {}", e.what());
        ac::logger::logg(error);
    }
}

template<typename... Args>
void ac::Logger::logg_and_print(const char* format_string, Args&&... args) {
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
        log_stream << formatted_string << std::endl;
        ac::print(formatted_string);
    }
    catch (const std::format_error& e) {
        std::string error = std::format("Format error: {}", e.what());
        ac::logger::logg(error);
    }
    catch (const std::invalid_argument& e) {
        std::string error = std::format("Invalid argument: {}", e.what());
        ac::logger::logg(error);
    }
}

template<typename... Args>
void ac::Logger::loggnl_and_printnl(const char* format_string, Args&&... args) {
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
        log_stream << formatted_string;
        ac::printnl(formatted_string);
    }
    catch (const std::format_error& e) {
        std::string error = std::format("Format error: {}", e.what());
        ac::logger::logg(error);
    }
    catch (const std::invalid_argument& e) {
        std::string error = std::format("Invalid argument: {}", e.what());
        ac::logger::logg(error);
    }
}

template<typename... Args>
void ac::Logger::logg_and_logg(const char* format_string, Args&&... args) {
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
        log_stream << formatted_string << std::endl;
        ac::logger::logg(formatted_string);
    }
    catch (const std::format_error& e) {
        std::string error = std::format("Format error: {}", e.what());
        ac::logger::logg(error);
    }
    catch (const std::invalid_argument& e) {
        std::string error = std::format("Invalid argument: {}", e.what());
        ac::logger::logg(error);
    }
}

template<typename... Args>
void ac::Logger::loggnl_and_loggnl(const char* format_string, Args&&... args) {
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
        log_stream << formatted_string;
        ac::logger::loggnl(formatted_string);
    }
    catch (const std::format_error& e) {
        std::string error = std::format("Format error: {}", e.what());
        ac::logger::logg(error);
    }
    catch (const std::invalid_argument& e) {
        std::string error = std::format("Invalid argument: {}", e.what());
        ac::logger::logg(error);
    }
}
