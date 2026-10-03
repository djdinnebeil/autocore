/**
 * \file journal_episode_format.ixx
 * \brief Zero-padding for a numeric episode count.
 *
 * The stored count stays an integer. This helper is the only display rule.
 */
export module journal_episode_format;

import std;

export namespace journal {

/**
 * \brief Prints `number` at least `padding` digits wide.
 *
 * Widths of 0 and 1 do not add a leading zero. A number with more digits
 * than `padding` is printed in full.
 */
[[nodiscard]] inline std::string format_episode_number(
    const int number,
    const int padding
) {
    const std::string digits = std::to_string(number);
    if (padding <= static_cast<int>(digits.size())) {
        return digits;
    }
    return std::string(static_cast<std::size_t>(padding) - digits.size(), '0') +
        digits;
}

} // namespace journal
