/**
 * \file journal_auto_select.ixx
 * \brief Accepted `[journal] auto_select_new_series` values.
 *
 * Trimmed ASCII `on` selects the new series. Trimmed ASCII `off` keeps the
 * current active name. A missing key or any other value stays on.
 */
export module journal_auto_select;

import std;

export namespace journal::auto_select {

[[nodiscard]] inline bool enabled(const std::optional<std::string_view> value) {
    if (!value) {
        return true;
    }
    const auto first = value->find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return true;
    }
    const auto last = value->find_last_not_of(" \t");
    const std::string_view trimmed = value->substr(first, last - first + 1);
    std::string folded;
    folded.reserve(trimmed.size());
    for (const unsigned char character : trimmed) {
        folded.push_back(
            character >= 'A' && character <= 'Z'
                ? static_cast<char>(character + ('a' - 'A'))
                : static_cast<char>(character)
        );
    }
    if (folded == "off") {
        return false;
    }
    return true;
}

} // namespace journal::auto_select
