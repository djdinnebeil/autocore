/**
 * \file song_template.ixx
 * \brief Generic `{token}` template parse, compile, and apply.
 *
 * Spotify field names and JSON rendering live in `song_catalog.ixx`.
 */
module;

#include <Windows.h>

export module spotify_song_template;

import std;

export namespace spotify::song {

inline constexpr std::string_view default_template =
    "[{name}] [{duration}] [{artist}] [{album}]";

inline constexpr std::size_t max_template_bytes = 4096;

struct Piece {
    bool is_token = false;
    std::string literal;
    std::string token;
};

struct Compiled {
    std::vector<Piece> pieces;

    [[nodiscard]] bool references(std::string_view token) const {
        for (const Piece& piece : pieces) {
            if (piece.is_token && piece.token == token) {
                return true;
            }
        }
        return false;
    }
};

struct CompileResult {
    bool ok = false;
    std::string error;
    Compiled compiled;
};

struct LoadedSongFormat {
    bool invalid = false;
    std::string error;
    Compiled compiled;
};

[[nodiscard]] CompileResult compile_utf8(
    std::string_view bytes,
    std::span<const std::string_view> known_tokens
);

[[nodiscard]] std::optional<std::string> accepted_song_format(
    std::string_view text,
    std::span<const std::string_view> known_tokens
);

[[nodiscard]] LoadedSongFormat load_song_format_file(
    const std::filesystem::path& path,
    std::span<const std::string_view> known_tokens
);

[[nodiscard]] bool replace_song_format(
    const std::filesystem::path& path,
    std::string_view contents
);

template <class Lookup>
[[nodiscard]] std::string apply(const Compiled& compiled, Lookup&& lookup) {
    std::string output;
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

} // namespace spotify::song

namespace {

[[nodiscard]] bool is_blank(std::string_view text) noexcept {
    if (text.empty()) {
        return true;
    }
    for (const unsigned char character : text) {
        if (character != ' ' && character != '\t') {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool is_identifier(std::string_view token) noexcept {
    if (token.empty()) {
        return false;
    }
    for (const unsigned char character : token) {
        const bool letter =
            (character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z');
        const bool digit = character >= '0' && character <= '9';
        if (!letter && !digit && character != '_') {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool is_known_token(
    std::string_view token,
    std::span<const std::string_view> known_tokens
) noexcept {
    for (const std::string_view known : known_tokens) {
        if (known == token) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::string_view without_trailing_newline(
    std::string_view text
) noexcept {
    if (text.ends_with("\r\n")) {
        text.remove_suffix(2);
    }
    else if (text.ends_with('\n') || text.ends_with('\r')) {
        text.remove_suffix(1);
    }
    return text;
}

[[nodiscard]] bool is_valid_utf8(std::string_view text) noexcept {
    if (text.empty()) {
        return true;
    }
    if (text.size() > static_cast<std::size_t>(INT_MAX)) {
        return false;
    }
    const int written = ::MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.data(),
        static_cast<int>(text.size()),
        nullptr,
        0
    );
    return written > 0;
}

} // namespace

namespace spotify::song {

namespace {

[[nodiscard]] CompileResult compile_utf8_impl(
    std::string_view bytes,
    std::span<const std::string_view> known_tokens,
    bool attaching_default
);

[[nodiscard]] CompileResult fail_compile(
    std::string error,
    std::span<const std::string_view> known_tokens,
    bool attaching_default
) {
    CompileResult result;
    result.error = std::move(error);
    if (!attaching_default) {
        const CompileResult fallback = compile_utf8_impl(
            default_template,
            known_tokens,
            true
        );
        if (fallback.ok) {
            result.compiled = fallback.compiled;
        }
    }
    return result;
}

[[nodiscard]] CompileResult compile_utf8_impl(
    std::string_view bytes,
    std::span<const std::string_view> known_tokens,
    bool attaching_default
) {
    if (bytes.size() > max_template_bytes) {
        return fail_compile(
            "Song format is larger than 4096 bytes.",
            known_tokens,
            attaching_default
        );
    }

    const std::string_view text = without_trailing_newline(bytes);
    if (text.find('\n') != std::string_view::npos ||
        text.find('\r') != std::string_view::npos) {
        return fail_compile(
            "Song format contains a newline.",
            known_tokens,
            attaching_default
        );
    }
    if (is_blank(text)) {
        return fail_compile(
            "Song format is empty.",
            known_tokens,
            attaching_default
        );
    }
    if (!is_valid_utf8(text)) {
        return fail_compile(
            "Song format is not valid UTF-8.",
            known_tokens,
            attaching_default
        );
    }

    CompileResult result;
    Piece literal;
    for (std::size_t index = 0; index < text.size(); ++index) {
        const char character = text[index];
        if (character != '{') {
            literal.literal.push_back(character);
            continue;
        }

        const std::size_t close = text.find('}', index + 1);
        if (close == std::string_view::npos) {
            return fail_compile(
                "Song format has an unclosed '{'.",
                known_tokens,
                attaching_default
            );
        }

        const std::string_view token = text.substr(index + 1, close - index - 1);
        if (token.empty()) {
            return fail_compile(
                "Song format has an empty token.",
                known_tokens,
                attaching_default
            );
        }
        for (const unsigned char token_character : token) {
            if (token_character == ' ' || token_character == '\t') {
                return fail_compile(
                    "Song format token contains whitespace.",
                    known_tokens,
                    attaching_default
                );
            }
        }

        if (!literal.literal.empty()) {
            result.compiled.pieces.push_back(std::move(literal));
            literal = {};
        }

        if (!is_identifier(token) || !is_known_token(token, known_tokens)) {
            return fail_compile(
                "Unknown song format token '{" + std::string {token} + "}'.",
                known_tokens,
                attaching_default
            );
        }

        Piece piece;
        piece.is_token = true;
        piece.token = std::string {token};
        result.compiled.pieces.push_back(std::move(piece));
        index = close;
    }

    if (!literal.literal.empty()) {
        result.compiled.pieces.push_back(std::move(literal));
    }
    result.ok = true;
    return result;
}

} // namespace

CompileResult compile_utf8(
    const std::string_view bytes,
    const std::span<const std::string_view> known_tokens
) {
    return compile_utf8_impl(bytes, known_tokens, false);
}

std::optional<std::string> accepted_song_format(
    const std::string_view text,
    const std::span<const std::string_view> known_tokens
) {
    if (text.size() > max_template_bytes) {
        return std::nullopt;
    }
    if (!compile_utf8(text, known_tokens).ok) {
        return std::nullopt;
    }
    return std::string {without_trailing_newline(text)};
}

LoadedSongFormat load_song_format_file(
    const std::filesystem::path& path,
    const std::span<const std::string_view> known_tokens
) {
    LoadedSongFormat loaded;
    const CompileResult fallback = compile_utf8(default_template, known_tokens);
    if (fallback.ok) {
        loaded.compiled = fallback.compiled;
    }

    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        loaded.invalid = true;
        loaded.error = "Failed to inspect " + path.string() + ".";
        return loaded;
    }
    if (!present) {
        return loaded;
    }

    const auto size = std::filesystem::file_size(path, error);
    if (error) {
        loaded.invalid = true;
        loaded.error = "Failed to read " + path.string() + ".";
        return loaded;
    }
    if (size > max_template_bytes) {
        loaded.invalid = true;
        loaded.error = "Song format is larger than 4096 bytes.";
        return loaded;
    }

    std::ifstream input(path, std::ios::binary);
    std::string bytes(static_cast<std::size_t>(size), '\0');
    if (size > 0) {
        input.read(bytes.data(), static_cast<std::streamsize>(size));
    }
    if (!input || input.gcount() != static_cast<std::streamsize>(size)) {
        loaded.invalid = true;
        loaded.error = "Failed to read " + path.string() + ".";
        return loaded;
    }

    const CompileResult compiled = compile_utf8(bytes, known_tokens);
    if (!compiled.ok) {
        loaded.invalid = true;
        loaded.error = compiled.error;
        return loaded;
    }
    loaded.compiled = compiled.compiled;
    return loaded;
}

bool replace_song_format(
    const std::filesystem::path& path,
    const std::string_view contents
) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return false;
    }

    const auto temporary_path = std::filesystem::path {path.wstring() + L".new"};
    {
        std::ofstream output(temporary_path, std::ios::binary | std::ios::trunc);
        if (!output) {
            return false;
        }
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.flush();
        if (!output) {
            output.close();
            ::DeleteFileW(temporary_path.c_str());
            return false;
        }
    }

    if (!::MoveFileExW(
            temporary_path.c_str(),
            path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        )) {
        ::DeleteFileW(temporary_path.c_str());
        return false;
    }
    return true;
}

} // namespace spotify::song
