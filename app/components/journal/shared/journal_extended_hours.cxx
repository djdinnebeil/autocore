/**
 * \file journal_extended_hours.cxx
 * \brief Implements Journal extended-hour cutoff tokens.
 */
module journal_extended_hours;

import auto_core.core.clock;
import journal_data_directory;

namespace {

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

} // namespace

namespace journal::extended_hours {

std::optional<ExtendedHours> parse_token(std::string_view token) {
    if (token.empty()) {
        return std::nullopt;
    }

    bool include_boundary = false;
    if (token.front() == '+') {
        include_boundary = true;
        token.remove_prefix(1);
        if (token.empty() || token.front() == '+') {
            return std::nullopt;
        }
    }
    if (token.front() == '-' || token.front() == '+') {
        return std::nullopt;
    }
    if (token.size() > 1 && token.front() == '0') {
        return std::nullopt;
    }

    int cutoff = 0;
    const auto result = std::from_chars(
        token.data(),
        token.data() + token.size(),
        cutoff
    );
    if (result.ec != std::errc {} ||
        result.ptr != token.data() + token.size() ||
        cutoff < 0 ||
        cutoff > 12) {
        return std::nullopt;
    }

    return ExtendedHours {cutoff, include_boundary};
}

std::string canonical_token(const ExtendedHours& hours) {
    const auto digits = std::to_string(hours.cutoff_hour);
    if (hours.include_boundary) {
        return "+" + digits;
    }
    return digits;
}

std::string file_text(const ExtendedHours& hours) {
    return "extended_hours = " + canonical_token(hours) + "\n";
}

std::optional<ExtendedHours> parse_file(std::string_view text) {
    std::optional<std::string> line;
    while (!text.empty()) {
        const auto break_at = text.find('\n');
        auto row = break_at == std::string_view::npos
            ? text
            : text.substr(0, break_at);
        if (break_at == std::string_view::npos) {
            text = {};
        }
        else {
            text.remove_prefix(break_at + 1);
        }
        if (!row.empty() && row.back() == '\r') {
            row.remove_suffix(1);
        }
        const auto trimmed = trim(row);
        if (trimmed.empty()) {
            continue;
        }
        if (line) {
            return std::nullopt;
        }
        line = std::string {trimmed};
    }
    if (!line) {
        return std::nullopt;
    }

    const auto separator = line->find('=');
    if (separator == std::string::npos) {
        return std::nullopt;
    }
    if (trim(std::string_view {*line}.substr(0, separator)) !=
        "extended_hours") {
        return std::nullopt;
    }
    return parse_token(trim(std::string_view {*line}.substr(separator + 1)));
}

std::filesystem::path file_path() {
    return journal::data_directory() / "extended_hour.clock";
}

ExtendedHours load() {
    std::ifstream input(file_path(), std::ios::binary);
    if (!input) {
        return {};
    }
    const std::string contents {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
    if (const auto parsed = parse_file(contents)) {
        return *parsed;
    }
    return {};
}

std::string format(const ExtendedHours& hours, const int hour, const int minute) {
    if (hours.include_boundary &&
        hour == hours.cutoff_hour &&
        minute == 0) {
        return std::format("{:02}:00", 24 + hours.cutoff_hour);
    }

    return ac::clock::format_extended_timestamp(
        hour,
        minute,
        hours.cutoff_hour
    );
}

std::string range_description() {
    std::string text;
    const auto append = [&text](
        const std::string_view token,
        const std::string_view meaning
    ) {
        text += std::format("{:<4} = {}\n", token, meaning);
    };

    append("0", "00:00-23:59");
    append("+0", "24:00, then 00:01-23:59");
    text.push_back('\n');

    for (int cutoff = 1; cutoff <= 12; ++cutoff) {
        const auto token = std::to_string(cutoff);
        const auto extended_end = 23 + cutoff;
        const auto boundary_hour = 24 + cutoff;
        append(
            token,
            std::format(
                "24:00-{:02}:59, then {:02}:00-23:59",
                extended_end,
                cutoff
            )
        );
        append(
            "+" + token,
            std::format(
                "24:00-{:02}:59, {:02}:00, then {:02}:01-23:59",
                extended_end,
                boundary_hour,
                cutoff
            )
        );
        if (cutoff != 12) {
            text.push_back('\n');
        }
    }
    return text;
}

} // namespace journal::extended_hours
