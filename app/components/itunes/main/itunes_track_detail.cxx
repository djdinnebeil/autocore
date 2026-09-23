#include "itunes_track_detail.hpp"

#include <iomanip>
#include <sstream>

namespace itunes::track::detail {

    std::wstring format_track(
        const std::wstring_view name,
        const std::wstring_view artist,
        const std::wstring_view album,
        const int duration_seconds
    ) {
        std::wostringstream duration;
        duration << duration_seconds / 60 << ':'
                 << std::setw(2) << std::setfill(L'0')
                 << duration_seconds % 60;

        std::wostringstream output;
        output << '[' << name << "] [" << artist << "] [" << album
               << "] [" << duration.str() << ']';
        return output.str();
    }

    bool record_history(
        std::vector<std::wstring>& history,
        std::wstring& last_track,
        const std::wstring_view current_track
    ) {
        const bool changed = last_track != current_track;
        if (changed) {
            history.emplace_back(current_track);
        }
        last_track = current_track;
        return changed;
    }

    std::wstring drain_history(std::vector<std::wstring>& history) {
        if (history.empty()) {
            return L"\n";
        }

        std::wstring output;
        for (const auto& track : history) {
            output += track;
            output += L'\n';
        }
        history.clear();
        return output;
    }

} // namespace itunes::track::detail
