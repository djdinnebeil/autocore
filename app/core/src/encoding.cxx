module encoding;

import std;
import <Windows.h>;

namespace ac::encoding {

    std::string to_utf8(std::wstring_view text) {
        if (text.empty()) {
            return {};
        }

        if (text.size() >
            static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
            throw std::length_error("Text is too large to convert");
        }

        const int input_size = static_cast<int>(text.size());
        constexpr DWORD conversion_flags = WC_ERR_INVALID_CHARS;

        const int output_size = WideCharToMultiByte(
            CP_UTF8,
            conversion_flags,
            text.data(),
            input_size,
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (output_size == 0) {
            const DWORD error_code = GetLastError();

            throw std::runtime_error(
                std::format(
                    "UTF-16 to UTF-8 conversion failed with error {}",
                    error_code
                )
            );
        }

        std::string result(
            static_cast<std::size_t>(output_size),
            '\0'
        );

        const int converted_size = WideCharToMultiByte(
            CP_UTF8,
            conversion_flags,
            text.data(),
            input_size,
            result.data(),
            output_size,
            nullptr,
            nullptr
        );

        if (converted_size == 0) {
            const DWORD error_code = GetLastError();

            throw std::runtime_error(
                std::format(
                    "UTF-16 to UTF-8 conversion failed with error {}",
                    error_code
                )
            );
        }

        return result;
    }

    std::string to_utf8(wchar_t character) {
        return to_utf8(std::wstring_view {&character, 1});
    }

    std::wstring to_utf16(std::string_view text) {
        if (text.empty()) {
            return {};
        }

        if (text.size() >
            static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
            throw std::length_error("Text is too large to convert");
        }

        const int input_size = static_cast<int>(text.size());
        constexpr DWORD conversion_flags = 0; // Use the default Windows UTF-8 conversion behavior.

        const int output_size = MultiByteToWideChar(
            CP_UTF8,
            conversion_flags,
            text.data(),
            input_size,
            nullptr,
            0
        );

        if (output_size == 0) {
            const DWORD error_code = GetLastError();

            throw std::runtime_error(
                std::format(
                    "UTF-8 to UTF-16 conversion failed with error {}",
                    error_code
                )
            );
        }

        std::wstring result(
            static_cast<std::size_t>(output_size),
            L'\0'
        );

        const int converted_size = MultiByteToWideChar(
            CP_UTF8,
            conversion_flags,
            text.data(),
            input_size,
            result.data(),
            output_size
        );

        if (converted_size == 0) {
            const DWORD error_code = GetLastError();

            throw std::runtime_error(
                std::format(
                    "UTF-8 to UTF-16 conversion failed with error {}",
                    error_code
                )
            );
        }

        return result;
    }

}