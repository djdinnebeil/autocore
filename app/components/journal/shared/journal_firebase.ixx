/**
 * \file journal_firebase.ixx
 * \brief `firebase.id` text and the Firebase JSON body.
 *
 * HTTP stays in `journal_cloud.exe`. This module only handles the file and
 * the payload string.
 */
export module journal_firebase;

import std;
import journal_data_directory;

export namespace journal::firebase {

[[nodiscard]] inline std::filesystem::path file_path() {
    return journal::data_directory() / "firebase.id";
}

[[nodiscard]] inline std::string trim_copy(const std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return std::string {value.substr(first, last - first + 1)};
}

[[nodiscard]] inline std::string json_escape(const std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const unsigned char character : value) {
        switch (character) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            if (character < 32) {
                constexpr char digits[] = "0123456789abcdef";
                escaped += "\\u00";
                escaped.push_back(digits[character >> 4]);
                escaped.push_back(digits[character & 0x0f]);
            }
            else {
                escaped.push_back(static_cast<char>(character));
            }
            break;
        }
    }
    return escaped;
}

[[nodiscard]] inline std::string payload(const std::string_view name, const int count) {
    return std::format(
        "{{\"name\":\"{}\",\"count\":{}}}",
        json_escape(name),
        count
    );
}

/**
 * \brief Reads the single URL from `firebase.id`.
 *
 * A missing file, blank or whitespace-only content, or an additional
 * nonblank line is no endpoint. A trailing newline is allowed.
 */
[[nodiscard]] inline std::optional<std::string> read_url(
    const std::filesystem::path& path
) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    const std::string url = trim_copy(buffer.str());
    if (url.empty() || url.find('\n') != std::string::npos ||
        url.find('\r') != std::string::npos) {
        return std::nullopt;
    }
    return url;
}

/**
 * \brief Creates an empty `firebase.id`.
 *
 * Blank content means Firebase is not configured. This is not a URL write:
 * `write_url` still rejects a blank endpoint.
 */
[[nodiscard]] inline std::expected<void, std::string> write_blank(
    const std::filesystem::path& path
) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return std::unexpected(
            std::format("Unable to create {}: {}", path.parent_path().string(), error.message())
        );
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return std::unexpected(std::format("Unable to write {}.", path.string()));
    }
    output.close();
    if (!output) {
        return std::unexpected(std::format("Unable to write {}.", path.string()));
    }
    return {};
}

[[nodiscard]] inline std::expected<void, std::string> write_url(
    const std::filesystem::path& path,
    const std::string_view url
) {
    const std::string trimmed = trim_copy(url);
    if (trimmed.empty() || trimmed.find('\n') != std::string::npos ||
        trimmed.find('\r') != std::string::npos) {
        return std::unexpected("Firebase URL must not be blank.");
    }
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return std::unexpected(
            std::format("Unable to create {}: {}", path.parent_path().string(), error.message())
        );
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return std::unexpected(std::format("Unable to write {}.", path.string()));
    }
    output << trimmed << '\n';
    output.close();
    if (!output) {
        return std::unexpected(std::format("Unable to write {}.", path.string()));
    }
    return {};
}

[[nodiscard]] inline std::string menu_text(const std::optional<std::string>& url) {
    if (!url || url->empty()) {
        return
            "Journal Firebase\n"
            "\n"
            "1. Add Firebase URL\n"
            "2. Exit\n";
    }
    return std::format(
        "Journal Firebase\n"
        "\n"
        "Current Firebase URL:\n"
        "{}\n"
        "\n"
        "1. Add new Firebase URL\n"
        "2. Exit\n",
        *url
    );
}

} // namespace journal::firebase
