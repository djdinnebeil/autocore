/**
\file utils.ixx
\brief Provides utility functions.

This module contains various utility functions used throughout the application,
including std::string conversions and base64 encoding.
*/
module;

#ifdef BUILDING_DLL
    #define DLL_API __declspec(dllexport)
#else
    #define DLL_API __declspec(dllimport)
#endif

export module utils;
import std;
import <Windows.h>;

export namespace ac::utils {
    DLL_API std::string wstr_to_str(const std::wstring& wstr);
    DLL_API std::wstring str_to_wstr(const std::string& str);
    DLL_API std::string base64_encode(const std::string& in);
    DLL_API std::string remove_first_and_last_char(const std::string& input);
}
