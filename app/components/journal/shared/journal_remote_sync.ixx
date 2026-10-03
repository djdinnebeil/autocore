/**
 * \file journal_remote_sync.ixx
 * \brief Accepted `[journal] remote_sync` values.
 *
 * Trimmed ASCII case-insensitive `on` turns the cloud runtime on. Trimmed
 * ASCII case-insensitive `off` leaves it off. Every other value, including a
 * missing key, leaves it off.
 */
export module journal_remote_sync;

import std;

export namespace journal::remote_sync {

[[nodiscard]] inline std::optional<std::string> canonical(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return std::nullopt;
    }
    const auto last = value.find_last_not_of(" \t");
    value = value.substr(first, last - first + 1);
    std::string folded;
    folded.reserve(value.size());
    for (const unsigned char character : value) {
        folded.push_back(
            character >= 'A' && character <= 'Z'
                ? static_cast<char>(character + ('a' - 'A'))
                : static_cast<char>(character)
        );
    }
    if (folded == "on" || folded == "off") {
        return folded;
    }
    return std::nullopt;
}

[[nodiscard]] inline bool enabled(std::string_view value) {
    const auto token = canonical(value);
    return token.has_value() && *token == "on";
}

} // namespace journal::remote_sync
