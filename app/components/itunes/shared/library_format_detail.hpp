/**
 * \file library_format_detail.hpp
 * \brief Load and serialize `library.format`.
 *
 * The file controls how many left-to-right columns are retained from
 * tab-delimited iTunes Library rows copied through the Clipboard, and how
 * those columns are rendered. `column` is the only reserved word.
 */
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace itunes::library_format::detail {

    inline constexpr int default_column_count = 4;
    inline constexpr std::string_view default_library_format = "[column]";

    enum class LibraryFormatMode {
        surround,
        separate
    };

    struct CompiledLibraryFormat {
        LibraryFormatMode mode = LibraryFormatMode::surround;
        char left_symbol = '[';
        char right_symbol = ']';
        std::string separator;
    };

    struct LibraryFormat {
        std::string format_text {default_library_format};
        CompiledLibraryFormat compiled {};
        int column_count = default_column_count;
    };

    struct LoadedLibraryFormat {
        LibraryFormat format {};
        std::optional<std::string> error;
    };

    /**
     * \brief Parses one positive `column_count` token.
     *
     * The whole string must be an integer greater than or equal to 1.
     */
    [[nodiscard]] std::optional<int> parse_column_count(std::string_view text);

    /**
     * \brief Compiles one library format expression.
     *
     * One `column` token is surrounding mode: exactly one byte, `column`,
     * and one byte. Two `column` tokens are separator mode: nothing outside
     * the tokens, and exactly one non-space, non-tab character between them.
     * Spaces and tabs around that character are kept. Anything else is
     * rejected.
     */
    [[nodiscard]] std::optional<CompiledLibraryFormat> compile_library_format(
        std::string_view text
    );

    /**
     * \brief Serializes `format` then `column_count`, ending after the
     * second line's newline.
     */
    [[nodiscard]] std::string library_format_text(const LibraryFormat& format);

    /**
     * \brief Loads `library.format`.
     *
     * A nonexistent file resolves both fields to their compiled defaults
     * with no error and is not created. An existing file that cannot be
     * read returns both defaults plus an error. A missing assignment uses
     * that field's default with no error. An invalid assignment falls back
     * only that field and sets an error.
     */
    [[nodiscard]] LoadedLibraryFormat load_library_format(
        const std::filesystem::path& path
    );

} // namespace itunes::library_format::detail
