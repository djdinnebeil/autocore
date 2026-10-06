#include "itunes_formatting_detail.hpp"

#include <utility>
#include <vector>

namespace itunes::formatting::detail {

    namespace {

        [[nodiscard]] std::vector<std::string> retained_columns(
            const std::string_view input,
            const int column_count
        ) {
            std::vector<std::string> fields;
            std::string current;
            int tab_number = 0;
            bool stopped = false;

            for (const char character : input) {
                if (character == '\t') {
                    ++tab_number;
                    if (tab_number == column_count) {
                        fields.push_back(std::move(current));
                        stopped = true;
                        break;
                    }
                    fields.push_back(std::move(current));
                    current.clear();
                }
                else if (character != '\r') {
                    current.push_back(character);
                }
            }

            if (!stopped) {
                fields.push_back(std::move(current));
            }
            return fields;
        }

        [[nodiscard]] std::string apply_format(
            const std::vector<std::string>& fields,
            const itunes::library_format::detail::CompiledLibraryFormat& format
        ) {
            using itunes::library_format::detail::LibraryFormatMode;

            std::string output;
            for (std::size_t index = 0; index < fields.size(); ++index) {
                if (format.mode == LibraryFormatMode::surround) {
                    if (index != 0) {
                        output.push_back(' ');
                    }
                    output.push_back(format.left_symbol);
                    output += fields[index];
                    output.push_back(format.right_symbol);
                }
                else {
                    if (index != 0) {
                        output += format.separator;
                    }
                    output += fields[index];
                }
            }
            return output;
        }

    } // namespace

    std::string format_queue_item(
        const std::string_view input,
        const itunes::library_format::detail::CompiledLibraryFormat& format,
        const int column_count
    ) {
        return apply_format(retained_columns(input, column_count), format);
    }

} // namespace itunes::formatting::detail
