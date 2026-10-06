/**
 * \file ini.cxx
 * \brief Implements sectioned INI parsing.
 */
module auto_core.core.ini;

import std;

namespace ac::ini {

    namespace {
        std::string_view trim(const std::string_view value) noexcept {
            const auto first = value.find_first_not_of(" \t\r");
            if (first == std::string_view::npos) {
                return {};
            }

            const auto last = value.find_last_not_of(" \t\r");
            return value.substr(first, last - first + 1);
        }
    }

    std::optional<std::string_view> Document::find(
        const std::string_view section,
        const std::string_view key
    ) const {
        const auto found_section = sections_.find(std::string {section});
        if (found_section == sections_.end()) {
            return std::nullopt;
        }

        for (const auto& setting : found_section->second) {
            if (setting.first == key) {
                return setting.second;
            }
        }

        return std::nullopt;
    }

    std::vector<std::pair<std::string, std::string>> Document::settings(
        const std::string_view section
    ) const {
        const auto found_section = sections_.find(std::string {section});
        if (found_section == sections_.end()) {
            return {};
        }

        return found_section->second;
    }

    Document parse(const std::string_view text) {
        Document document;
        std::string current_section;
        std::size_t line_start = 0;

        while (line_start <= text.size()) {
            const auto line_end = text.find('\n', line_start);
            const auto length = line_end == std::string_view::npos
                ? text.size() - line_start
                : line_end - line_start;
            const std::string_view line =
                trim(text.substr(line_start, length));

            if (!line.empty() && !line.starts_with(';') &&
                !line.starts_with('#')) {
                if (line.starts_with('[') && line.ends_with(']')) {
                    current_section = std::string {
                        trim(line.substr(1, line.size() - 2))
                    };
                }
                else if (!current_section.empty()) {
                    const auto equals = line.find('=');
                    if (equals != std::string_view::npos) {
                        const std::string_view key =
                            trim(line.substr(0, equals));
                        if (!key.empty()) {
                            auto& section =
                                document.sections_[current_section];
                            std::string key_text {key};
                            std::string value_text {
                                trim(line.substr(equals + 1))
                            };

                            bool replaced = false;
                            for (auto& setting : section) {
                                if (setting.first == key_text) {
                                    setting.second = std::move(value_text);
                                    replaced = true;
                                    break;
                                }
                            }

                            if (!replaced) {
                                section.emplace_back(
                                    std::move(key_text),
                                    std::move(value_text)
                                );
                            }
                        }
                    }
                }
            }

            if (line_end == std::string_view::npos) {
                break;
            }
            line_start = line_end + 1;
        }

        return document;
    }

    std::optional<Document> read(
        const std::filesystem::path& path
    ) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            return std::nullopt;
        }

        const std::string text {
            std::istreambuf_iterator<char> {input},
            std::istreambuf_iterator<char> {}
        };
        if (input.bad()) {
            return std::nullopt;
        }

        return parse(text);
    }

} // namespace ac::ini
