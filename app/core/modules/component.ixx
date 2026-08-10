/**
 * \file component.ixx
 * \brief Defines component-scoped output for Auto Core.
 *
 * Component owns the output identity and component log for one Auto Core
 * component. Public operations describe where an event is routed while the
 * mechanism used to reach the main log remains an implementation detail.
 */
module;

#include "ac_api.hpp"

export module component;

import std;
import encoding;

export namespace ac {

    class Component {
    public:
        AC_API explicit Component(const std::string& name);
        AC_API ~Component() noexcept;

        Component(const Component&) = delete;
        Component& operator=(const Component&) = delete;
        Component(Component&&) = delete;
        Component& operator=(Component&&) = delete;

        // Component log only.
        AC_API void logg(const std::string& message);
        AC_API void loggnl(const std::string& message);

        // Component log + main log.
        AC_API void logg_and_logg(const std::string& message);
        AC_API void loggnl_and_loggnl(const std::string& message);

        // Component log + main log + console.
        AC_API void logg_and_print(const std::string& message);
        AC_API void loggnl_and_printnl(const std::string& message);
        AC_API void print(const std::string& message);
        AC_API void printnl(const std::string& message);

        // Component log + main log + console + insertion into the active input.
        AC_API void print_and_insert(const std::string& message);

        AC_API void logg(const std::wstring& message);
        AC_API void loggnl(const std::wstring& message);
        AC_API void logg_and_logg(const std::wstring& message);
        AC_API void loggnl_and_loggnl(const std::wstring& message);
        AC_API void logg_and_print(const std::wstring& message);
        AC_API void loggnl_and_printnl(const std::wstring& message);
        AC_API void print(const std::wstring& message);
        AC_API void printnl(const std::wstring& message);
        AC_API void print_and_insert(const std::wstring& message);

        AC_API void logg(char message);
        AC_API void loggnl(char message);
        AC_API void logg_and_logg(char message);
        AC_API void loggnl_and_loggnl(char message);
        AC_API void logg_and_print(char message);
        AC_API void loggnl_and_printnl(char message);
        AC_API void print(char message);
        AC_API void printnl(char message);

        AC_API void logg(wchar_t message);
        AC_API void loggnl(wchar_t message);
        AC_API void logg_and_logg(wchar_t message);
        AC_API void loggnl_and_loggnl(wchar_t message);
        AC_API void logg_and_print(wchar_t message);
        AC_API void loggnl_and_printnl(wchar_t message);
        AC_API void print(wchar_t message);
        AC_API void printnl(wchar_t message);

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

        template<typename... Args>
        void print(const char* format_string, Args&&... args);

        template<typename... Args>
        void printnl(const char* format_string, Args&&... args);

        template<typename... Args>
        void print_and_insert(const char* format_string, Args&&... args);

        AC_API void update_log_file();
        AC_API void flush();

    private:
        class Impl;
        std::unique_ptr<Impl> impl;

        template<typename... Args>
        std::optional<std::string> format_message(
            const char* format_string,
            Args&&... args
        );
    };
}

namespace ac {

    template<typename... Args>
    std::optional<std::string> Component::format_message(
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
                    std::is_same_v<Arg, wchar_t*> ||
                    std::is_same_v<Arg, std::wstring> ||
                    std::is_same_v<Arg, std::wstring_view> ||
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
            logg_and_print(std::format(
                "Component format error: {}",
                exception.what()
            ));
        }
        catch (const std::invalid_argument& exception) {
            logg_and_print(std::format(
                "Component invalid argument: {}",
                exception.what()
            ));
        }

        return std::nullopt;
    }

    template<typename... Args>
    void Component::logg(const char* format_string, Args&&... args) {
        if (auto message = format_message(
            format_string,
            std::forward<Args>(args)...
        )) {
            logg(*message);
        }
    }

    template<typename... Args>
    void Component::loggnl(const char* format_string, Args&&... args) {
        if (auto message = format_message(
            format_string,
            std::forward<Args>(args)...
        )) {
            loggnl(*message);
        }
    }

    template<typename... Args>
    void Component::logg_and_logg(
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
    void Component::loggnl_and_loggnl(
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
    void Component::logg_and_print(
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
    void Component::loggnl_and_printnl(
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

    template<typename... Args>
    void Component::print(const char* format_string, Args&&... args) {
        if (auto message = format_message(
            format_string,
            std::forward<Args>(args)...
        )) {
            print(*message);
        }
    }

    template<typename... Args>
    void Component::printnl(const char* format_string, Args&&... args) {
        if (auto message = format_message(
            format_string,
            std::forward<Args>(args)...
        )) {
            printnl(*message);
        }
    }

    template<typename... Args>
    void Component::print_and_insert(
        const char* format_string,
        Args&&... args
    ) {
        if (auto message = format_message(
            format_string,
            std::forward<Args>(args)...
        )) {
            print_and_insert(*message);
        }
    }
}
