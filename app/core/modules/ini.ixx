/**
 * \file ini.ixx
 * \brief Parses sectioned INI configuration documents.
 */
module;

#include "ac_api.hpp"

export module auto_core.ini;

import std;

export namespace ac::ini {

    class Document;

    /**
     * \brief Parses INI text.
     *
     * Blank lines and lines beginning with `;` or `#` are ignored. Settings
     * outside a section and malformed lines are ignored.
     *
     * \param text The complete INI document.
     * \return The parsed settings.
     */
    [[nodiscard]]
    AC_API Document parse(std::string_view text);

    /**
     * \brief An immutable collection of INI sections and settings.
     *
     * Section and key names are case-sensitive. Whitespace surrounding
     * section names, keys, and values is ignored. When a key occurs more
     * than once in a section, the last value wins.
     */
    class Document {
    public:
        /**
         * \brief Finds a setting without copying its value.
         * \return The stored value, including an empty value, or
         * `std::nullopt` when the section or key is absent.
         */
        [[nodiscard]]
        AC_API std::optional<std::string_view> find(
            std::string_view section,
            std::string_view key
        ) const noexcept;

    private:
        using Section = std::unordered_map<std::string, std::string>;
        std::unordered_map<std::string, Section> sections_;

        friend AC_API Document parse(std::string_view text);
    };

    /**
     * \brief Reads and parses an INI file.
     * \param path The file to read.
     * \return The parsed document, or `std::nullopt` when the file cannot be
     * opened or read.
     */
    [[nodiscard]]
    AC_API std::optional<Document> read(
        const std::filesystem::path& path
    );

} // namespace ac::ini
