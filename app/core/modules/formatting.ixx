/**
 * \file formatting.ixx
 * \brief Provides UTF-8 formatting with argument normalization.
 *
 * This module performs formatting only. Callers retain responsibility for
 * reporting failures and routing the resulting message.
 */
export module auto_core.formatting;

import std;
import auto_core.encoding;

namespace ac::formatting::detail {

    template<typename T>
    auto normalize_argument(T&& argument) {
        // Decay preserves formatter behavior for string literals:
        // wchar_t arrays become wchar_t pointers before type selection.
        using Arg = std::decay_t<T>;

        if constexpr (std::is_same_v<Arg, std::filesystem::path>) {
            return ac::encoding::to_utf8(argument.native());
        }
        else if constexpr (std::is_same_v<Arg, wchar_t>) {
            return ac::encoding::to_utf8(argument);
        }
        else if constexpr (
            std::is_same_v<Arg, std::wstring> ||
            std::is_same_v<Arg, std::wstring_view>
        ) {
            return ac::encoding::to_utf8(argument);
        }
        else if constexpr (
            std::is_same_v<Arg, const wchar_t*> ||
            std::is_same_v<Arg, wchar_t*>
        ) {
            if (argument == nullptr) {
                return std::string {"(null)"};
            }

            return ac::encoding::to_utf8(
                std::wstring_view {argument}
            );
        }
        else if constexpr (
            std::is_same_v<Arg, const char*> ||
            std::is_same_v<Arg, char*>
        ) {
            if (argument == nullptr) {
                return std::string {"(null)"};
            }

            return std::string {argument};
        }
        else {
            return std::forward<T>(argument);
        }
    }
} // namespace ac::formatting::detail

export namespace ac::formatting {

    /**
     * \brief Formats a UTF-8 string with normalized text arguments.
     *
     * The format string uses `std::format` syntax. All arguments are
     * normalized before formatting, allowing narrow and supported wide-text
     * values to be used together in one UTF-8 result. Null narrow and wide C
     * strings are formatted as `(null)`.
     *
     * \param format_string A null-terminated UTF-8 format string.
     * \param args Values referenced by the replacement fields.
     * \return The formatted UTF-8 string.
     * \throws std::invalid_argument if `format_string` is null.
     * \throws std::format_error if the format string or its argument usage is
     * invalid.
     * \throws std::length_error if converted text exceeds the encoding
     * module's supported input size.
     * \throws std::runtime_error if conversion of a wide-text argument fails.
     */
    template<typename... Args>
    [[nodiscard]]
    std::string format(
        const char* format_string,
        Args&&... args
    ) {
        if (format_string == nullptr) {
            throw std::invalid_argument("Format string is null");
        }

        auto normalized_arguments = std::make_tuple(
            detail::normalize_argument(std::forward<Args>(args))...
        );

        return std::apply(
            [&](auto&&... normalized) {
                return std::vformat(
                    format_string,
                    std::make_format_args(normalized...)
                );
            },
            normalized_arguments
        );
    }

} // namespace ac::formatting
