/**
 * \file encoding.ixx
 * \brief Provides text encoding conversion utilities.
 */

module;

#if defined(BUILDING_DLL)
    #define DLL_API __declspec(dllexport)
#else
    #define DLL_API __declspec(dllimport)
#endif

export module encoding;

import std;

export namespace ac::encoding {

    DLL_API std::string to_utf8(const std::wstring& text);
    DLL_API std::string to_utf8(wchar_t character);

}