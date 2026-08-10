/**
 * \file logger_c.ixx
 * \brief Defines the component-specific logger.
 *
 * Logger writes messages to an individual component log and can optionally
 * route the same message through the main logger or the print system.
 */
module;

#include "ac_api.hpp"

export module logger;

import std;
import encoding;
import error;

export namespace ac {

    class Logger {
    public:
        AC_API explicit Logger(const std::string& log_name);
        AC_API ~Logger() noexcept;

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        AC_API void update_log_file();
        AC_API void flush();

        AC_API void logg(const std::string& msg);
        AC_API void loggnl(const std::string& msg);
        AC_API void logg_and_logg(const std::string& msg);
        AC_API void loggnl_and_loggnl(const std::string& msg);
        AC_API void logg_and_print(const std::string& msg);
        AC_API void loggnl_and_printnl(const std::string& msg);

        AC_API void logg(const std::wstring& msg);
        AC_API void loggnl(const std::wstring& msg);
        AC_API void logg_and_logg(const std::wstring& msg);
        AC_API void loggnl_and_loggnl(const std::wstring& msg);
        AC_API void logg_and_print(const std::wstring& msg);
        AC_API void loggnl_and_printnl(const std::wstring& msg);

        AC_API void logg(char msg);
        AC_API void loggnl(char msg);
        AC_API void logg_and_logg(char msg);
        AC_API void loggnl_and_loggnl(char msg);
        AC_API void logg_and_print(char msg);
        AC_API void loggnl_and_printnl(char msg);

        AC_API void logg(wchar_t msg);
        AC_API void loggnl(wchar_t msg);
        AC_API void logg_and_logg(wchar_t msg);
        AC_API void loggnl_and_loggnl(wchar_t msg);
        AC_API void logg_and_print(wchar_t msg);
        AC_API void loggnl_and_printnl(wchar_t msg);

        template<typename... Args>
        void logg(const char* format_string, Args&&... args);

        template<typename... Args>
        void loggnl(const char* format_string, Args&&... args);

        template<typename... Args>
        void logg_and_logg(const char* format_string, Args&&... args);

        template<typename... Args>
        void loggnl_and_loggnl(const char* format_string, Args&&... args);

        template<typename... Args>
        void logg_and_print(const char* format_string, Args&&... args);

        template<typename... Args>
        void loggnl_and_printnl(const char* format_string, Args&&... args);

    private:
        class Impl;

        template<typename... Args>
        std::optional<std::string> format_message(
            const char* format_string,
            Args&&... args
        );

        void open_log_file();
        void close_log_file();

        std::unique_ptr<Impl> impl;
    };
}

namespace ac {

    template<typename... Args>
    std::optional<std::string> Logger::format_message(
        const char* format_string,
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

            return std::apply(
                [&](auto&&... unpacked_args) {
                    return std::vformat(
                        format_string,
                        std::make_format_args(unpacked_args...)
                    );
                },
                converted_args
            );
        }
        catch (const std::format_error& exception) {
            ac::error::log(
                "Component logger format error: {}",
                exception.what()
            );
        }
        catch (const std::invalid_argument& exception) {
            ac::error::log(
                "Component logger invalid argument: {}",
                exception.what()
            );
        }

        return std::nullopt;
    }

    template<typename... Args>
    void Logger::logg(const char* format_string, Args&&... args) {
        if (auto message = format_message(
            format_string,
            std::forward<Args>(args)...
        )) {
            logg(*message);
        }
    }

    template<typename... Args>
    void Logger::loggnl(const char* format_string, Args&&... args) {
        if (auto message = format_message(
            format_string,
            std::forward<Args>(args)...
        )) {
            loggnl(*message);
        }
    }

    template<typename... Args>
    void Logger::logg_and_logg(
        const char* format_string,
        Args&&... args
    ) {
        if (auto message = format_message(
            format_string,
            std::forward<Args>(args)...
        )) {
            logg_and_logg(*message);
        }
    }

    template<typename... Args>
    void Logger::loggnl_and_loggnl(
        const char* format_string,
        Args&&... args
    ) {
        if (auto message = format_message(
            format_string,
            std::forward<Args>(args)...
        )) {
            loggnl_and_loggnl(*message);
        }
    }

    template<typename... Args>
    void Logger::logg_and_print(
        const char* format_string,
        Args&&... args
    ) {
        if (auto message = format_message(
            format_string,
            std::forward<Args>(args)...
        )) {
            logg_and_print(*message);
        }
    }

    template<typename... Args>
    void Logger::loggnl_and_printnl(
        const char* format_string,
        Args&&... args
    ) {
        if (auto message = format_message(
            format_string,
            std::forward<Args>(args)...
        )) {
            loggnl_and_printnl(*message);
        }
    }
}
