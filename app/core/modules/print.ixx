/**
\file print.ixx
\brief This module provides functionality for sending output to the console.

This module includes functions to print messages to the console, with support
for formatted std::strings. It also handles logging of printed messages and interaction
with the clipboard.
*/
module;

#include "ac_api.hpp"

export module print;

import std;
import encoding;

export namespace ac {

    AC_API void print(const std::string& msg);
    AC_API void print(const std::wstring& msg);
    AC_API void print(char msg);
    AC_API void print(wchar_t msg);

    AC_API void printnl(const std::string& msg);
    AC_API void printnl(const std::wstring& msg);
    AC_API void printnl(char msg);
    AC_API void printnl(wchar_t msg);

    AC_API void print_and_insert(const std::string& msg);
    AC_API void print_and_insert(const std::wstring& msg);
    AC_API void insert_text(const std::string& msg);
    AC_API void insert_text(const std::wstring& msg);

    template<typename... Args>
    void print(const char* format_string, Args&&... args);

    template<typename... Args>
    void printnl(const char* format_string, Args&&... args);

}

namespace ac {

    using print_func = void(*)(const std::string&);

    template<typename... Args>
    void print_template(
        const char* format_string,
        print_func destination,
        Args&&... args
    ) {
        try {
            if (format_string == nullptr) {
                throw std::invalid_argument("Format string is null");
            }

            auto convert_arg = [](auto&& arg) -> decltype(auto) {
                using Arg = std::decay_t<decltype(arg)>;

                if constexpr (
                    std::is_same_v<Arg, const wchar_t*> ||
                    std::is_same_v<Arg, std::wstring> ||
                    std::is_same_v<Arg, wchar_t>
                    ) {
                    return ac::encoding::to_utf8(
                        std::forward<decltype(arg)>(arg)
                    );
                }
                else {
                    return std::forward<decltype(arg)>(arg);
                }
                };

            auto converted_args = std::make_tuple(
                convert_arg(std::forward<Args>(args))...
            );

            const std::string formatted_message = std::apply(
                [&](auto&&... unpacked_args) {
                    return std::vformat(
                        format_string,
                        std::make_format_args(unpacked_args...)
                    );
                },
                converted_args
            );

            destination(formatted_message);
        }
        catch (const std::format_error& exception) {
            print(std::format(
                "Print format error: {}",
                exception.what()
            ));
        }
        catch (const std::invalid_argument& exception) {
            print(std::format(
                "Print invalid argument: {}",
                exception.what()
            ));
        }
    }

    template<typename... Args>
    void print(const char* format_string, Args&&... args) {
        print_template(
            format_string,
            static_cast<print_func>(print),
            std::forward<Args>(args)...
        );
    }

    template<typename... Args>
    void printnl(const char* format_string, Args&&... args) {
        print_template(
            format_string,
            static_cast<print_func>(printnl),
            std::forward<Args>(args)...
        );
    }

}
