/**
 * \file song_template_detail.hpp
 * \brief Generic `{token}` template parse, compile, and apply.
 *
 * iTunes property names and COM types live in `itunes_metadata_detail.hpp`.
 */
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace itunes::song::detail {

    inline constexpr std::string_view default_template =
        "[{name}] [{artist}] [{album}] [{duration}]";

    inline constexpr std::size_t max_template_bytes = 4096;

    struct Piece {
        bool is_token = false;
        std::wstring literal;
        std::string token;
    };

    struct Compiled {
        std::vector<Piece> pieces;

        [[nodiscard]] bool references(std::string_view token) const;
    };

    struct CompileResult {
        bool ok = false;
        std::string error;
        Compiled compiled;
    };

    [[nodiscard]] CompileResult compile(std::wstring_view text);

    /**
     * \brief Validates UTF-8 template bytes.
     *
     * Strips one trailing LF or CRLF. Embedded newlines, an empty template,
     * a template larger than `max_template_bytes`, malformed UTF-8, and
     * unknown tokens fail. Failure does not produce replacement bytes.
     */
    [[nodiscard]] CompileResult compile_utf8(std::string_view bytes);

    /**
     * \brief Returns the bytes to persist when `text` is a valid template.
     *
     * One trailing newline is removed. Invalid text returns `nullopt` so
     * the caller leaves any existing file unchanged.
     */
    [[nodiscard]] std::optional<std::string> accepted_song_format(
        std::string_view text
    );

    struct LoadedSongFormat {
        bool invalid = false;
        std::string error;
        Compiled compiled;
    };

    /**
     * \brief Loads `song.format`.
     *
     * A missing file uses the compiled default and is not invalid. An
     * unreadable or invalid file is invalid, uses the compiled default,
     * and is not rewritten.
     */
    [[nodiscard]] LoadedSongFormat load_song_format_file(
        const std::filesystem::path& path
    );

    [[nodiscard]] const Compiled& default_compiled();

    template <class Lookup>
    [[nodiscard]] std::wstring apply(
        const Compiled& compiled,
        Lookup&& lookup
    ) {
        std::wstring output;
        for (const Piece& piece : compiled.pieces) {
            if (piece.is_token) {
                output += lookup(std::string_view {piece.token});
            }
            else {
                output += piece.literal;
            }
        }
        return output;
    }

} // namespace itunes::song::detail
