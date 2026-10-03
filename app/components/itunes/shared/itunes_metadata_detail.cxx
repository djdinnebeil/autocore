#include "itunes_metadata_detail.hpp"

#include <iomanip>
#include <sstream>

namespace itunes::metadata::detail {

    std::wstring render_duration(const int duration_seconds) {
        std::wostringstream duration;
        duration << duration_seconds / 60 << L':'
                 << std::setw(2) << std::setfill(L'0')
                 << duration_seconds % 60;
        return duration.str();
    }

} // namespace itunes::metadata::detail
