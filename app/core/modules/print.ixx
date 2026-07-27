/**
\file print.ixx
\brief This module provides functionality for sending output to the console.

This module includes functions to print messages to the console, with support
for formatted std::strings. It also handles logging of printed messages and interaction
with the clipboard.
*/
module;

#ifdef BUILDING_DLL
    #define DLL_API __declspec(dllexport)
#else
    #define DLL_API __declspec(dllimport)
#endif

export module print;
import std;
import config;
import encoding;
import logger;
import clipboard;
import <Windows.h>;

export namespace ac {
    DLL_API void print(const std::string& msg);
    DLL_API void printnl(const std::string& msg);
    DLL_API void print(const std::wstring& msg);
    DLL_API void printnl(const std::wstring& msg);
    DLL_API void print(char msg);
    DLL_API void printnl(char msg);
    DLL_API void print(wchar_t msg);
    DLL_API void printnl(wchar_t msg);
    DLL_API void print_to_screen(const std::string& msg);
    DLL_API void print_to_screen_w(const std::wstring& msg);
    template<typename... Args> void print(const char* format_string, Args&&... args);
    template<typename... Args> void printnl(const char* format_string, Args&&... args);
}

namespace ac {
    using print_func = void(*)(const std::string&);

    template<typename... Args>
    void print_template(const char* format_string, print_func println, Args&&... args) {
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

            println(formatted_string);
        }
        catch (const std::format_error& e) {
            std::string error = std::format("Format error: {}", e.what());
            print(error);
        }
        catch (const std::invalid_argument& e) {
            std::string error = std::format("Invalid argument: {}", e.what());
            print(error);
        }
    }
}

template<typename... Args>
void ac::print(const char* format_string, Args&&... args) {
    print_template(
        format_string,
        static_cast<print_func>(ac::print),
        std::forward<Args>(args)...
    );
}

template<typename... Args>
void ac::printnl(const char* format_string, Args&&... args) {
    print_template(
        format_string,
        static_cast<print_func>(ac::printnl),
        std::forward<Args>(args)...
    );
}
