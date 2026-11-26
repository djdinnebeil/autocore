/**
 * \file component_log_format_detail.hpp
 * \brief Formats persisted component log records.
 */
#pragma once

#include <ostream>
#include <string_view>

namespace ac::component_detail {

    inline bool write_log_record(
        std::ostream& stream,
        bool& line_open,
        const std::string_view timestamp,
        const std::string_view event_type,
        const std::string_view message,
        const bool newline,
        const std::string_view area = {}
    ) {
        if (!stream.good()) {
            return false;
        }

        const auto start_line = [&] {
            stream << '[' << timestamp << "] ";
            if (!area.empty()) {
                stream << '[' << area << "] ";
            }
            if (!event_type.empty()) {
                stream << '[' << event_type << "] ";
            }
            line_open = true;
        };

        std::size_t offset = 0;
        while (offset < message.size()) {
            if (!line_open) {
                start_line();
            }

            const std::size_t end = message.find('\n', offset);
            if (end == std::string_view::npos) {
                stream << message.substr(offset);
                offset = message.size();
            }
            else {
                stream << message.substr(offset, end - offset) << '\n';
                line_open = false;
                offset = end + 1;
            }
        }

        if (message.empty() && !line_open) {
            start_line();
        }

        if (newline && line_open) {
            stream << '\n';
            line_open = false;
        }

        if (newline) {
            stream.flush();
        }

        return stream.good();
    }

} // namespace ac::component_detail
