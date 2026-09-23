/**
 * \file itunes_track_detail.hpp
 * \brief Pure formatting and history operations for iTunes tracks.
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace itunes::track::detail {

    [[nodiscard]] std::wstring format_track(
        std::wstring_view name,
        std::wstring_view artist,
        std::wstring_view album,
        int duration_seconds
    );

    [[nodiscard]] bool record_history(
        std::vector<std::wstring>& history,
        std::wstring& last_track,
        std::wstring_view current_track
    );

    [[nodiscard]] std::wstring drain_history(
        std::vector<std::wstring>& history
    );

} // namespace itunes::track::detail
