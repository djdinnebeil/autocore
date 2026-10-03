#include "song_template_detail.hpp"
#include "itunes_metadata_detail.hpp"

#include <Windows.h>

#include <fstream>

namespace itunes::song::detail {

    namespace {

        [[nodiscard]] bool is_token_whitespace(wchar_t character) noexcept {
            return character == L' ' || character == L'\t';
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

        [[nodiscard]] bool is_supported_token(std::string_view token) noexcept {
            for (const auto& spec : itunes::metadata::detail::catalog) {
                if (spec.token == token) {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] std::optional<std::wstring> utf8_to_wide(
            std::string_view text
        ) {
            if (text.empty()) {
                return std::wstring {};
            }
            if (text.size() > static_cast<std::size_t>(INT_MAX)) {
                return std::nullopt;
            }
            const int written = ::MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                text.data(),
                static_cast<int>(text.size()),
                nullptr,
                0
            );
            if (written <= 0) {
                return std::nullopt;
            }
            std::wstring wide(static_cast<std::size_t>(written), L'\0');
            const int converted = ::MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                text.data(),
                static_cast<int>(text.size()),
                wide.data(),
                written
            );
            if (converted <= 0) {
                return std::nullopt;
            }
            return wide;
        }

        [[nodiscard]] std::wstring widen_ascii(std::string_view text) {
            std::wstring wide;
            wide.reserve(text.size());
            for (const unsigned char character : text) {
                wide.push_back(static_cast<wchar_t>(character));
            }
            return wide;
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

        [[nodiscard]] CompileResult reject(std::string error) {
            CompileResult result;
            result.error = std::move(error);
            result.compiled = default_compiled();
            return result;
        }

    } // namespace

    bool Compiled::references(const std::string_view token) const {
        for (const Piece& piece : pieces) {
            if (piece.is_token && piece.token == token) {
                return true;
            }
        }
        return false;
    }

    CompileResult compile(const std::wstring_view text) {
        CompileResult result;
        if (text.empty()) {
            result.error = "Song format is empty.";
            return result;
        }
        if (text.find(L'\n') != std::wstring_view::npos ||
            text.find(L'\r') != std::wstring_view::npos) {
            result.error = "Song format contains a newline.";
            return result;
        }

        Piece literal;
        for (std::size_t index = 0; index < text.size(); ++index) {
            const wchar_t character = text[index];
            if (character != L'{') {
                literal.literal.push_back(character);
                continue;
            }

            const std::size_t close = text.find(L'}', index + 1);
            if (close == std::wstring_view::npos) {
                result.error = "Song format has an unclosed '{'.";
                return result;
            }

            const std::wstring_view token = text.substr(index + 1, close - index - 1);
            if (token.empty()) {
                result.error = "Song format has an empty token.";
                return result;
            }
            for (const wchar_t token_character : token) {
                if (is_token_whitespace(token_character)) {
                    result.error = "Song format token contains whitespace.";
                    return result;
                }
            }

            if (!literal.literal.empty()) {
                result.compiled.pieces.push_back(std::move(literal));
                literal = {};
            }

            Piece piece;
            piece.is_token = true;
            piece.token.reserve(token.size());
            for (const wchar_t token_character : token) {
                if (token_character < 33 || token_character > 126) {
                    result.error = "Unknown song format token.";
                    return result;
                }
                piece.token.push_back(static_cast<char>(token_character));
            }
            if (!is_identifier(piece.token)) {
                result.error = "Unknown song format token '{" + piece.token + "}'.";
                return result;
            }
            result.compiled.pieces.push_back(std::move(piece));
            index = close;
        }

        if (!literal.literal.empty()) {
            result.compiled.pieces.push_back(std::move(literal));
        }
        result.ok = true;
        return result;
    }

    CompileResult compile_utf8(const std::string_view bytes) {
        if (bytes.size() > max_template_bytes) {
            return reject("Song format is larger than 4096 bytes.");
        }

        const std::string_view text = without_trailing_newline(bytes);
        if (text.find('\n') != std::string_view::npos ||
            text.find('\r') != std::string_view::npos) {
            return reject("Song format contains a newline.");
        }
        if (text.empty()) {
            return reject("Song format is empty.");
        }

        const auto wide = utf8_to_wide(text);
        if (!wide) {
            return reject("Song format is not valid UTF-8.");
        }

        CompileResult result = compile(*wide);
        if (!result.ok) {
            result.compiled = default_compiled();
            return result;
        }
        for (const Piece& piece : result.compiled.pieces) {
            if (piece.is_token && !is_supported_token(piece.token)) {
                result.ok = false;
                result.error = "Unknown song format token '{" + piece.token + "}'.";
                result.compiled = default_compiled();
                return result;
            }
        }
        return result;
    }

    std::optional<std::string> accepted_song_format(const std::string_view text) {
        if (text.size() > max_template_bytes) {
            return std::nullopt;
        }
        const std::string_view body = without_trailing_newline(text);
        if (!compile_utf8(body).ok) {
            return std::nullopt;
        }
        return std::string {body};
    }

    LoadedSongFormat load_song_format_file(const std::filesystem::path& path) {
        LoadedSongFormat loaded;
        loaded.compiled = default_compiled();

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

        const CompileResult compiled = compile_utf8(bytes);
        if (!compiled.ok) {
            loaded.invalid = true;
            loaded.error = compiled.error;
            return loaded;
        }
        loaded.compiled = compiled.compiled;
        return loaded;
    }

    const Compiled& default_compiled() {
        static const Compiled compiled = [] {
            const CompileResult result = compile(widen_ascii(default_template));
            return result.ok ? result.compiled : Compiled {};
        }();
        return compiled;
    }

} // namespace itunes::song::detail
