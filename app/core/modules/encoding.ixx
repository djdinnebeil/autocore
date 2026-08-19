/**
 * \file encoding.ixx
 * \brief Provides text encoding conversion utilities.
 */
module;

#include "ac_api.hpp"

export module auto_core.encoding;

import std;

export namespace ac::encoding {

    /**
     * \brief Converts one UTF-16 code unit to UTF-8.
     *
     * This overload is intended for characters represented by a single
     * UTF-16 code unit. A surrogate code unit is not a complete character
     * and causes conversion to fail; use the string-view overload for a
     * supplementary character represented by a surrogate pair.
     *
     * \param character The UTF-16 code unit to encode.
     * \return The UTF-8 encoded bytes.
     * \throws std::runtime_error if the code unit is not valid on its own or
     * the Windows conversion operation otherwise fails.
     */
    AC_API std::string to_utf8(wchar_t character);

    /**
     * \brief Converts UTF-16 text to UTF-8.
     *
     * Conversion is strict: unpaired surrogate code units are rejected.
     * Embedded null characters are converted and preserved, and an empty
     * input produces an empty string.
     *
     * \param text The UTF-16 text to convert.
     * \return The UTF-8 encoded bytes.
     * \throws std::length_error if the input contains more than INT_MAX code
     * units.
     * \throws std::runtime_error if the input is not well-formed UTF-16 or
     * the Windows conversion operation otherwise fails.
     */
    AC_API std::string to_utf8(std::wstring_view text);

    /**
     * \brief Converts UTF-8 text to UTF-16.
     *
     * Conversion is strict: malformed UTF-8 byte sequences are rejected.
     * Embedded null characters are converted and preserved, and an empty
     * input produces an empty string.
     *
     * \param text The UTF-8 bytes to convert.
     * \return The UTF-16 encoded text.
     * \throws std::length_error if the input contains more than INT_MAX
     * bytes.
     * \throws std::runtime_error if the input is not well-formed UTF-8 or the
     * Windows conversion operation otherwise fails.
     */
    AC_API std::wstring to_utf16(std::string_view text);
} // namespace ac::encoding
