#include "library_format_detail.hpp"

#include <charconv>
#include <fstream>
#include <vector>

namespace itunes::library_format::detail {

    namespace {

        constexpr std::size_t max_format_bytes = 4096;
        constexpr std::string_view column_token = "column";

        [[nodiscard]] std::string_view trim(std::string_view value) noexcept {
            const auto first = value.find_first_not_of(" \t\r");
            if (first == std::string_view::npos) {
                return {};
            }
            const auto last = value.find_last_not_of(" \t\r");
            return value.substr(first, last - first + 1);
        }

        [[nodiscard]] std::vector<std::size_t> column_tokens(
            const std::string_view text
        ) {
            std::vector<std::size_t> positions;
            std::size_t search = 0;
            while (search <= text.size()) {
                const auto found = text.find(column_token, search);
                if (found == std::string_view::npos) {
                    break;
                }
                positions.push_back(found);
                search = found + column_token.size();
            }
            return positions;
        }

        [[nodiscard]] bool one_separator_symbol(
            const std::string_view between
        ) noexcept {
            int symbols = 0;
            for (const char character : between) {
                if (character != ' ' && character != '\t') {
                    ++symbols;
                }
            }
            return symbols == 1;
        }

        void restore_default_format(LibraryFormat& format) {
            format.format_text = std::string {default_library_format};
            format.compiled = {};
        }

        [[nodiscard]] LoadedLibraryFormat parse_bytes(
            const std::string_view bytes,
            const std::filesystem::path& path
        ) {
            LoadedLibraryFormat loaded;
            bool format_invalid = false;
            bool count_invalid = false;
            std::size_t line_start = 0;

            while (line_start <= bytes.size()) {
                const auto line_end = bytes.find('\n', line_start);
                const auto length = line_end == std::string_view::npos
                    ? bytes.size() - line_start
                    : line_end - line_start;
                const std::string_view line =
                    trim(bytes.substr(line_start, length));

                if (!line.empty()) {
                    const auto equals = line.find('=');
                    if (equals != std::string_view::npos) {
                        const auto key = trim(line.substr(0, equals));
                        const auto value = trim(line.substr(equals + 1));
                        if (key == "format") {
                            if (const auto compiled = compile_library_format(value)) {
                                loaded.format.format_text = std::string {value};
                                loaded.format.compiled = *compiled;
                            }
                            else {
                                format_invalid = true;
                            }
                        }
                        else if (key == "column_count") {
                            if (const auto parsed = parse_column_count(value)) {
                                loaded.format.column_count = *parsed;
                            }
                            else {
                                count_invalid = true;
                            }
                        }
                    }
                }

                if (line_end == std::string_view::npos) {
                    break;
                }
                line_start = line_end + 1;
            }

            if (format_invalid) {
                restore_default_format(loaded.format);
            }
            if (count_invalid) {
                loaded.format.column_count = default_column_count;
            }

            if (format_invalid && count_invalid) {
                loaded.error =
                    "Invalid format and column_count in " + path.string() + ".";
            }
            else if (format_invalid) {
                loaded.error = "Invalid format in " + path.string() + ".";
            }
            else if (count_invalid) {
                loaded.error = "Invalid column_count in " + path.string() + ".";
            }
            return loaded;
        }

        [[nodiscard]] LoadedLibraryFormat failed(std::string error) {
            LoadedLibraryFormat loaded;
            loaded.error = std::move(error);
            return loaded;
        }

    } // namespace

    std::optional<int> parse_column_count(const std::string_view text) {
        if (text.empty()) {
            return std::nullopt;
        }

        int value = 0;
        const auto parsed = std::from_chars(
            text.data(),
            text.data() + text.size(),
            value
        );
        if (parsed.ec != std::errc {} ||
            parsed.ptr != text.data() + text.size() ||
            value < 1) {
            return std::nullopt;
        }
        return value;
    }

    std::optional<CompiledLibraryFormat> compile_library_format(
        const std::string_view text
    ) {
        const auto positions = column_tokens(text);
        if (positions.size() == 1 &&
            positions[0] == 1 &&
            text.size() == column_token.size() + 2) {
            CompiledLibraryFormat compiled;
            compiled.mode = LibraryFormatMode::surround;
            compiled.left_symbol = text.front();
            compiled.right_symbol = text.back();
            return compiled;
        }

        if (positions.size() == 2 &&
            positions[0] == 0 &&
            positions[1] + column_token.size() == text.size()) {
            const auto between = text.substr(
                column_token.size(),
                positions[1] - column_token.size()
            );
            if (!one_separator_symbol(between)) {
                return std::nullopt;
            }
            CompiledLibraryFormat compiled;
            compiled.mode = LibraryFormatMode::separate;
            compiled.separator = std::string {between};
            return compiled;
        }

        return std::nullopt;
    }

    std::string library_format_text(const LibraryFormat& format) {
        std::string text = "format = ";
        text += format.format_text;
        text += "\ncolumn_count = ";
        text += std::to_string(format.column_count);
        text += '\n';
        return text;
    }

    LoadedLibraryFormat load_library_format(const std::filesystem::path& path) {
        std::error_code error;
        const bool present = std::filesystem::exists(path, error);
        if (error) {
            return failed("Failed to inspect " + path.string() + ".");
        }
        if (!present) {
            return {};
        }

        const bool regular = std::filesystem::is_regular_file(path, error);
        if (error || !regular) {
            return failed("Failed to read " + path.string() + ".");
        }

        const auto size = std::filesystem::file_size(path, error);
        if (error) {
            return failed("Failed to read " + path.string() + ".");
        }
        if (size > max_format_bytes) {
            return failed("Library format is larger than 4096 bytes.");
        }

        std::ifstream input(path, std::ios::binary);
        std::string bytes(static_cast<std::size_t>(size), '\0');
        if (size > 0) {
            input.read(bytes.data(), static_cast<std::streamsize>(size));
        }
        if (!input || input.gcount() != static_cast<std::streamsize>(size)) {
            return failed("Failed to read " + path.string() + ".");
        }

        return parse_bytes(bytes, path);
    }

} // namespace itunes::library_format::detail
