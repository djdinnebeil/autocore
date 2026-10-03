/**
 * \file wake_history_detail.cxx
 * \brief Applies one Wake capture to the canonical history files.
 */
#include "wake_history_detail.hpp"

#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace wake::history {
namespace {

[[nodiscard]] std::filesystem::path history_path(
    const std::filesystem::path& directory,
    const std::string_view name
) {
    return directory / std::string {name};
}

[[nodiscard]] std::optional<bool> path_exists(const std::filesystem::path& path) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        return std::nullopt;
    }
    return present;
}

[[nodiscard]] std::optional<std::string> read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad()) {
        return std::nullopt;
    }
    return buffer.str();
}

[[nodiscard]] bool write_file(
    const std::filesystem::path& path,
    const std::string_view contents
) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );
    output.close();
    return static_cast<bool>(output);
}

[[nodiscard]] bool append_file(
    const std::filesystem::path& path,
    const std::string_view contents
) {
    std::ofstream output(path, std::ios::binary | std::ios::app);
    if (!output) {
        return false;
    }
    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );
    output.close();
    return static_cast<bool>(output);
}

} // namespace

std::string normalize_body(const std::string_view captured) {
    std::string normalized;
    if (captured.empty()) {
        return normalized;
    }

    std::size_t begin = 0;
    while (begin < captured.size()) {
        const auto end = captured.find('\n', begin);
        std::string_view line = end == std::string_view::npos
            ? captured.substr(begin)
            : captured.substr(begin, end - begin);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        normalized.append(line);
        normalized.push_back('\n');
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1;
    }
    return normalized;
}

ApplyResult apply_capture(
    const std::filesystem::path& directory,
    const std::string_view captured,
    const std::string_view timestamp
) {
    const std::string body = normalize_body(captured);
    const auto current_path = history_path(directory, current_event_name);
    const auto previous_path = history_path(directory, previous_event_name);
    const auto log_path = history_path(directory, events_log_name);

    const auto previous_exists = path_exists(previous_path);
    const auto log_exists = path_exists(log_path);
    if (!previous_exists || !log_exists) {
        return ApplyResult::failed;
    }

    std::string previous_text;
    if (*previous_exists) {
        const auto previous = read_file(previous_path);
        if (!previous) {
            return ApplyResult::failed;
        }
        previous_text = normalize_body(*previous);
    }
    if (*log_exists) {
        if (!read_file(log_path)) {
            return ApplyResult::failed;
        }
    }

    const bool baseline_missing = !*previous_exists;
    const bool same = *previous_exists && previous_text == body;
    bool append = false;
    bool write_previous = false;
    bool create_empty_log = false;

    if (baseline_missing && !*log_exists) {
        if (body.empty()) {
            write_previous = true;
            create_empty_log = true;
        }
        else {
            append = true;
            write_previous = true;
        }
    }
    else if (baseline_missing) {
        write_previous = true;
    }
    else if (!same) {
        append = true;
        write_previous = true;
    }
    else if (!*log_exists) {
        create_empty_log = true;
    }

    if (!write_file(current_path, body)) {
        return ApplyResult::failed;
    }
    if (append) {
        const std::string entry = std::string {timestamp} + "\n" + body;
        if (!append_file(log_path, entry)) {
            return ApplyResult::failed;
        }
    }
    else if (create_empty_log) {
        if (!write_file(log_path, {})) {
            return ApplyResult::failed;
        }
    }
    if (write_previous) {
        if (!write_file(previous_path, body)) {
            return ApplyResult::failed;
        }
    }
    return append ? ApplyResult::recorded : ApplyResult::unchanged;
}

bool create_missing_history_files(const std::filesystem::path& directory) {
    std::error_code create_error;
    std::filesystem::create_directories(directory, create_error);
    if (create_error) {
        return false;
    }

    const std::string_view names[] {
        current_event_name,
        previous_event_name,
        events_log_name
    };
    for (const std::string_view name : names) {
        const auto path = history_path(directory, name);
        const auto present = path_exists(path);
        if (!present) {
            return false;
        }
        if (*present) {
            continue;
        }
        if (!write_file(path, {})) {
            return false;
        }
    }
    return true;
}

} // namespace wake::history
