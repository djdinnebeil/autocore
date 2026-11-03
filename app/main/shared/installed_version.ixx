/**
 * \file installed_version.ixx
 * \brief Keeps `[auto_core] version` equal to the compiled version.
 */
export module auto_core.main.installed_version;

import std;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.version;

export namespace ac::main {

    /**
     * Replaces a differing `[auto_core] version` with the compiled version.
     * A differing `# version` comment value is updated the same way.
     * Every other line is preserved. Matching values are not written.
     *
     * \return False when the file cannot be read or written.
     */
    [[nodiscard]]
    bool synchronize_installed_version() {
        namespace cfg = ac::main::config;
        const auto path = ac::paths::config_directory() / "auto_core.ini";
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            return false;
        }
        std::string text {
            std::istreambuf_iterator<char> {input},
            std::istreambuf_iterator<char> {}
        };
        if (input.bad()) {
            return false;
        }

        const std::string_view compiled = ac::version::auto_core_version;
        struct Replacement {
            std::size_t begin;
            std::size_t end;
            std::string text;
        };
        std::vector<Replacement> replacements;
        const auto note_version_value = [&](
            const std::string_view raw,
            const std::size_t value_line_start,
            const std::size_t equals
        ) {
            const auto value = cfg::trim(raw.substr(equals + 1));
            if (value == compiled) {
                return;
            }
            const auto value_region = raw.substr(equals + 1);
            const auto first = value_region.find_first_not_of(" \t");
            if (first == std::string_view::npos) {
                replacements.push_back({
                    .begin = value_line_start + equals + 1,
                    .end = value_line_start + raw.size(),
                    .text = std::string {" "} + std::string {compiled},
                });
                return;
            }
            const auto last = value_region.find_last_not_of(" \t");
            replacements.push_back({
                .begin = value_line_start + equals + 1 + first,
                .end = value_line_start + equals + 1 + last + 1,
                .text = std::string {compiled},
            });
        };
        std::string section;
        bool saw_section = false;
        bool saw_version = false;
        std::size_t insert_at = 0;
        bool header_has_newline = false;
        std::size_t line_start = 0;

        while (line_start <= text.size()) {
            const auto line_break = text.find('\n', line_start);
            const auto raw_end = line_break == std::string::npos
                ? text.size()
                : line_break;
            std::string_view raw {
                text.data() + line_start,
                raw_end - line_start
            };
            if (!raw.empty() && raw.back() == '\r') {
                raw.remove_suffix(1);
            }
            const auto trimmed = cfg::trim(raw);
            if (trimmed.starts_with('#')) {
                const auto comment = cfg::trim(trimmed.substr(1));
                const auto equals = comment.find('=');
                if (equals != std::string_view::npos &&
                    cfg::trim(comment.substr(0, equals)) == "version") {
                    const auto raw_equals = raw.find('=');
                    if (raw_equals != std::string_view::npos) {
                        note_version_value(raw, line_start, raw_equals);
                    }
                }
            }
            else if (!trimmed.empty() && !trimmed.starts_with(';')) {
                if (trimmed.starts_with('[') && trimmed.ends_with(']')) {
                    section = std::string {
                        cfg::trim(trimmed.substr(1, trimmed.size() - 2))
                    };
                    if (section == "auto_core") {
                        saw_section = true;
                        header_has_newline = line_break != std::string::npos;
                        insert_at = header_has_newline
                            ? line_break + 1
                            : text.size();
                    }
                }
                else if (section == "auto_core") {
                    const auto equals = raw.find('=');
                    if (equals != std::string_view::npos &&
                        cfg::trim(raw.substr(0, equals)) == "version") {
                        saw_version = true;
                        note_version_value(raw, line_start, equals);
                    }
                }
            }
            if (line_break == std::string::npos) {
                break;
            }
            line_start = line_break + 1;
        }

        for (auto replacement = replacements.rbegin();
             replacement != replacements.rend();
             ++replacement) {
            const auto old_length = replacement->end - replacement->begin;
            if (replacement->end <= insert_at) {
                const auto new_length = replacement->text.size();
                if (new_length >= old_length) {
                    insert_at += new_length - old_length;
                }
                else {
                    insert_at -= old_length - new_length;
                }
            }
            text.replace(
                replacement->begin,
                old_length,
                replacement->text
            );
        }

        if (!saw_version) {
            if (!saw_section) {
                if (replacements.empty()) {
                    return true;
                }
            }
            else {
                const bool crlf = text.find("\r\n") != std::string::npos;
                const std::string newline = crlf ? "\r\n" : "\n";
                std::string insertion;
                if (!header_has_newline) {
                    insertion += newline;
                }
                insertion += "version = ";
                insertion += compiled;
                insertion += newline;
                text.insert(insert_at, insertion);
            }
        }
        else if (replacements.empty()) {
            return true;
        }

        return cfg::write_bytes(path, text);
    }

} // namespace ac::main
