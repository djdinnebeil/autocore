/**
 * \file encoding.ixx
 * \brief Provides text encoding conversion utilities.
 */
module;

#include "ac_api.hpp"

export module encoding;

import std;

export namespace ac::encoding {
    AC_API std::string to_utf8(wchar_t character);
    AC_API std::string to_utf8(std::wstring_view text);
    AC_API std::wstring to_utf16(std::string_view text);
}
