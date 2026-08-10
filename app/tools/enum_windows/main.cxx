/**
 * \file main.cxx
 * \brief Enumerates visible Windows, displays their titles and class names,
 *        and reports selected Unicode characters and malformed UTF-16.
 *
 * This standalone diagnostic utility preserves window titles as UTF-16 and
 * supports Unicode titles from applications such as Spotify and iTunes.
 */
#include <Windows.h>

#include <cstdint>
#include <cwchar>
#include <iostream>
#include <string>
#include <string_view>

namespace {

    constexpr int max_class_name_length = 256;

    struct EnumerationContext {
        std::size_t window_count = 0;
    };

    bool is_high_surrogate(const wchar_t character) noexcept {
        return character >= 0xD800 && character <= 0xDBFF;
    }

    bool is_low_surrogate(const wchar_t character) noexcept {
        return character >= 0xDC00 && character <= 0xDFFF;
    }

    std::uint32_t combine_surrogates(
        const wchar_t high,
        const wchar_t low
    ) noexcept {
        return 0x10000
            + ((static_cast<std::uint32_t>(high) - 0xD800) << 10)
            + (static_cast<std::uint32_t>(low) - 0xDC00);
    }

    std::wstring format_code_point(const std::uint32_t code_point) {
        wchar_t buffer[16] {};

        swprintf_s(
            buffer,
            L"U+%04X",
            static_cast<unsigned int>(code_point)
        );

        return buffer;
    }

    void write_console(const std::wstring_view text) {
        if (text.empty()) {
            return;
        }

        const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);

        if (output == nullptr || output == INVALID_HANDLE_VALUE) {
            return;
        }

        DWORD console_mode {};

        if (GetConsoleMode(output, &console_mode) != FALSE) {
            DWORD characters_written {};

            WriteConsoleW(
                output,
                text.data(),
                static_cast<DWORD>(text.size()),
                &characters_written,
                nullptr
            );

            return;
        }

        // Output has been redirected. Convert UTF-16 to UTF-8.
        const int required_size = WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (required_size <= 0) {
            return;
        }

        std::string utf8(
            static_cast<std::size_t>(required_size),
            '\0'
        );

        const int converted_size = WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            utf8.data(),
            required_size,
            nullptr,
            nullptr
        );

        if (converted_size <= 0) {
            return;
        }

        utf8.resize(static_cast<std::size_t>(converted_size));

        DWORD bytes_written {};

        WriteFile(
            output,
            utf8.data(),
            static_cast<DWORD>(utf8.size()),
            &bytes_written,
            nullptr
        );
    }

    void write_console_line(std::wstring_view text = {}) {
        write_console(text);
        write_console(L"\n");
    }

    std::wstring retrieve_window_title(const HWND window) {
        const int title_length = GetWindowTextLengthW(window);

        if (title_length <= 0) {
            return {};
        }

        std::wstring title(
            static_cast<std::size_t>(title_length) + 1,
            L'\0'
        );

        const int copied_length = GetWindowTextW(
            window,
            title.data(),
            title_length + 1
        );

        if (copied_length <= 0) {
            return {};
        }

        title.resize(static_cast<std::size_t>(copied_length));
        return title;
    }

    std::wstring retrieve_window_class_name(const HWND window) {
        std::wstring class_name(
            static_cast<std::size_t>(max_class_name_length),
            L'\0'
        );

        const int copied_length = GetClassNameW(
            window,
            class_name.data(),
            max_class_name_length
        );

        if (copied_length <= 0) {
            return {};
        }

        class_name.resize(static_cast<std::size_t>(copied_length));
        return class_name;
    }

    void inspect_window_title(std::wstring_view title) {
        for (std::size_t index = 0; index < title.size(); ++index) {
            const wchar_t character = title[index];

            if (is_high_surrogate(character)) {
                if (
                    index + 1 < title.size() &&
                    is_low_surrogate(title[index + 1])
                    ) {
                    const std::uint32_t code_point =
                        combine_surrogates(character, title[index + 1]);

                    ++index; // The pair is valid. Preserve it and advance past both units.

                    write_console(L"***supplementary Unicode character [");
                    write_console(format_code_point(code_point));
                    write_console_line(L"] detected***");
                    
                    continue;
                }

                write_console_line(
                    L"***unpaired high surrogate detected***"
                );

                continue;
            }

            if (is_low_surrogate(character)) {
                write_console_line(
                    L"***unpaired low surrogate detected***"
                );

                continue;
            }

            const std::uint32_t code_point =
                static_cast<std::uint32_t>(character);

            switch (code_point) {
            case 0x2014:
                write_console_line(
                    L"***em dash [U+2014] detected***"
                );
                break;

            case 0x200B:
                write_console_line(
                    L"***zero-width space [U+200B] detected***"
                );
                break;

            case 0x200C:
                write_console_line(
                    L"***zero-width non-joiner [U+200C] detected***"
                );
                break;

            case 0x200D:
                write_console_line(
                    L"***zero-width joiner [U+200D] detected***"
                );
                break;

            case 0xFEFF:
                write_console_line(
                    L"***zero-width no-break space or BOM [U+FEFF] detected***"
                );
                break;

            default:
                break;
            }
        }
    }

    BOOL CALLBACK enumerate_window(
        const HWND window,
        const LPARAM parameter
    ) {
        if (IsWindowVisible(window) == FALSE) {
            return TRUE;
        }

        auto& context =
            *reinterpret_cast<EnumerationContext*>(parameter);

        ++context.window_count;

        const std::wstring title =
            retrieve_window_title(window);

        const std::wstring class_name =
            retrieve_window_class_name(window);

        write_console_line(
            std::to_wstring(context.window_count)
        );

        inspect_window_title(title);

        // Print the original title exactly as Windows returned it.
        write_console_line(title);
        write_console_line(class_name);
        write_console_line();

        return TRUE;
    }

} // namespace

int main() {
    EnumerationContext context;

    SetLastError(ERROR_SUCCESS);

    const BOOL succeeded = EnumWindows(
        enumerate_window,
        reinterpret_cast<LPARAM>(&context)
    );

    if (succeeded == FALSE) {
        const DWORD error_code = GetLastError();

        write_console(L"EnumWindows failed with error code ");
        write_console(std::to_wstring(error_code));
        write_console_line(L".");

        return 1;
    }

    write_console(L"Press Enter to exit...");
    std::cin.get();

    return 0;
}