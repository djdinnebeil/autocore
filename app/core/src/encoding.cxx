module encoding;

import std;
import <Windows.h>;

namespace ac::encoding {

    std::string to_utf8(const std::wstring& text)
    {
        int size_needed = WideCharToMultiByte(
            CP_UTF8,
            0,
            text.c_str(),
            static_cast<int>(text.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        std::string result(size_needed, 0);

        WideCharToMultiByte(
            CP_UTF8,
            0,
            text.c_str(),
            static_cast<int>(text.size()),
            result.data(),
            size_needed,
            nullptr,
            nullptr
        );

        return result;
    }

    std::string to_utf8(wchar_t character)
    {
        wchar_t text[] = {character, L'\0'};
        return to_utf8(text);
    }

}