#include "itunes_formatting_detail.hpp"

namespace itunes::formatting::detail {

    std::string format_queue_item(
        const std::string_view input,
        const int tab_end
    ) {
        std::string output {"["};
        int tab_number = 0;

        for (const char character : input) {
            if (character == '\t') {
                ++tab_number;
                if (tab_number == tab_end) {
                    break;
                }
                output += "] [";
            }
            else if (character != '\r') {
                output += character;
            }
        }

        output += ']';
        return output;
    }

} // namespace itunes::formatting::detail
