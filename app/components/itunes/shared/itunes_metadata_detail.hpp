/**
 * \file itunes_metadata_detail.hpp
 * \brief iTunes song-format tokens whose COM property, VARIANT type, and
 *        renderer have been checked against the installed iTunes interface.
 *
 * The supported set is Name, Artist, Album, and Duration. Location is
 * operational track data and is not a format token. Additional tokens stay
 * out of this catalog until a live COM probe confirms them.
 */
#pragma once

#include <string>
#include <string_view>

namespace itunes::metadata::detail {

    enum class ValueKind {
        bstr,
        duration_seconds
    };

    struct TokenSpec {
        std::string_view token;
        const wchar_t* com_property;
        ValueKind kind;
    };

    inline constexpr TokenSpec catalog[] {
        {"name", L"Name", ValueKind::bstr},
        {"artist", L"Artist", ValueKind::bstr},
        {"album", L"Album", ValueKind::bstr},
        {"duration", L"Duration", ValueKind::duration_seconds},
    };

    [[nodiscard]] std::wstring render_duration(int duration_seconds);

} // namespace itunes::metadata::detail
