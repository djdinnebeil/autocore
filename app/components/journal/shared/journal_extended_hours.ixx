/**
 * \file journal_extended_hours.ixx
 * \brief Journal extended-hour cutoff tokens.
 *
 * A token is a cutoff hour plus whether exactly `n:00` is included.
 * `n` extends while `hour < n`. `+n` does the same and adds exactly `n:00`.
 * This module owns that policy. The core clock formatter stays exclusive.
 */
export module journal_extended_hours;

import std;

export namespace journal::extended_hours {

    struct ExtendedHours {
        int cutoff_hour {0};
        bool include_boundary {false};

        [[nodiscard]]
        friend bool operator==(
            const ExtendedHours&,
            const ExtendedHours&
        ) = default;
    };

    /**
     * \brief Parses a canonical token.
     *
     * Accepts `0`, `+0`, `1`, `+1`, through `12`, `+12`.
     * Leading zeros and a discarded sign are rejected.
     */
    [[nodiscard]]
    std::optional<ExtendedHours> parse_token(std::string_view token);

    [[nodiscard]]
    std::string canonical_token(const ExtendedHours& hours);

    /** One line, no INI section: `extended_hours = <token>\\n`. */
    [[nodiscard]]
    std::string file_text(const ExtendedHours& hours);

    /**
     * \brief Parses one `extended_hours` line.
     *
     * Blank lines are ignored. Any other extra line is invalid.
     */
    [[nodiscard]]
    std::optional<ExtendedHours> parse_file(std::string_view text);

    [[nodiscard]]
    std::filesystem::path file_path();

    /**
     * \brief Reads `extended_hour.clock` in the journal data directory.
     *
     * A missing or malformed file becomes token `0`. This function
     * does not create or rewrite the file.
     */
    [[nodiscard]]
    ExtendedHours load();

    /**
     * \brief Formats one local `HH:MM` under the Journal token policy.
     *
     * An exact `+n` boundary (`hour == n` and `minute == 0`) renders
     * `(24 + n):00`. Every other minute uses
     * `ac::clock::detail::format_extended_timestamp`.
     */
    [[nodiscard]]
    std::string format(const ExtendedHours& hours, int hour, int minute);

    [[nodiscard]]
    std::string range_description();

} // namespace journal::extended_hours
