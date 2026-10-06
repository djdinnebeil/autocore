/**
 * \file journal_series_map.ixx
 * \brief Parse and render `series.map`.
 *
 * `journal_series.exe` is the only writer. Callers that only need the active
 * series use `read_active` and `choose_allocate`. Rendering does not read
 * snapshot rows back from a previous file.
 */
export module journal_series_map;

import std;
import journal_data_directory;
import journal_episode_format;

export namespace journal::series_map {

enum class ActiveKind {
    missing,
    blank,
    malformed,
    named,
};

struct ActiveField {
    ActiveKind kind = ActiveKind::missing;
    std::string name;
};

struct SnapshotSeries {
    std::string name;
    int next_episode {};
    int padding {};
};

/**
 * \brief How `journal_ac.exe` chooses the allocate key.
 *
 * Missing, blank, and malformed active fields send an empty key immediately.
 * A named field sends that name once. An unknown-series reply is retried by
 * the caller, not by this function.
 */
struct AllocateChoice {
    bool empty_key = true;
    std::string name;
};

[[nodiscard]] inline std::filesystem::path file_path() {
    return journal::data_directory() / "series.map";
}

[[nodiscard]] inline ActiveField parse_active(const std::string_view contents) {
    if (contents.empty()) {
        return {};
    }
    const auto newline = contents.find('\n');
    std::string_view line = newline == std::string_view::npos
        ? contents
        : contents.substr(0, newline);
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }
    constexpr std::string_view keyword = "active";
    if (line.size() < keyword.size() || line.substr(0, keyword.size()) != keyword) {
        return ActiveField {ActiveKind::malformed, {}};
    }
    std::string_view rest = line.substr(keyword.size());
    while (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) {
        rest.remove_prefix(1);
    }
    if (rest.empty() || rest.front() != '=') {
        return ActiveField {ActiveKind::malformed, {}};
    }
    rest.remove_prefix(1);
    while (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) {
        rest.remove_prefix(1);
    }
    while (!rest.empty() && (rest.back() == ' ' || rest.back() == '\t')) {
        rest.remove_suffix(1);
    }
    if (rest.empty()) {
        return ActiveField {ActiveKind::blank, {}};
    }
    return ActiveField {ActiveKind::named, std::string {rest}};
}

[[nodiscard]] inline ActiveField read_active(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input && !input.eof()) {
        return ActiveField {ActiveKind::malformed, {}};
    }
    return parse_active(buffer.str());
}

[[nodiscard]] inline AllocateChoice choose_allocate(const ActiveField& field) {
    if (field.kind != ActiveKind::named) {
        return {};
    }
    return AllocateChoice {false, field.name};
}

[[nodiscard]] inline std::string render(
    const std::string_view active_name,
    const std::span<const SnapshotSeries> rows
) {
    std::string text;
    text += "active = ";
    text += active_name;
    text += "\n\n[snapshot]\n";
    for (const SnapshotSeries& row : rows) {
        text += row.name;
        text += ' ';
        text += format_episode_number(row.next_episode, row.padding);
        text += '\n';
    }
    return text;
}

} // namespace journal::series_map
