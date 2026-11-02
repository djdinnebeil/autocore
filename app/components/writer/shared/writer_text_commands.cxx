module;

#include <Windows.h>

module writer_text_commands;

import std;
import auto_core.core.encoding;
import auto_core.core.paths;
import writer_protocol;

namespace writer::text_commands {

namespace {

const wchar_t* characters(const std::wstring_view text) {
    return text.empty() ? L"" : text.data();
}

bool extension_is_txt(const std::filesystem::path& path) {
    const std::wstring extension = path.extension().wstring();
    constexpr auto max_length =
        static_cast<std::size_t>((std::numeric_limits<int>::max)());
    if (extension.size() > max_length) {
        return false;
    }
    return CompareStringOrdinal(
        characters(extension),
        static_cast<int>(extension.size()),
        L".txt",
        4,
        TRUE
    ) == CSTR_EQUAL;
}

bool filename_less(
    const std::wstring_view left,
    const std::wstring_view right
) {
    constexpr auto max_length =
        static_cast<std::size_t>((std::numeric_limits<int>::max)());
    if (left.size() > max_length || right.size() > max_length) {
        if (left.size() != right.size()) {
            return left.size() < right.size();
        }
        return left < right;
    }

    const int folded = CompareStringOrdinal(
        characters(left),
        static_cast<int>(left.size()),
        characters(right),
        static_cast<int>(right.size()),
        TRUE
    );
    if (folded == CSTR_EQUAL) {
        return CompareStringOrdinal(
            characters(left),
            static_cast<int>(left.size()),
            characters(right),
            static_cast<int>(right.size()),
            FALSE
        ) == CSTR_LESS_THAN;
    }
    return folded == CSTR_LESS_THAN;
}

bool is_command_name(const std::string_view name) {
    if (name.empty()) {
        return false;
    }
    const auto head = static_cast<unsigned char>(name.front());
    if (!(std::isalpha(head) || name.front() == '_')) {
        return false;
    }
    for (const char character : name) {
        const auto value = static_cast<unsigned char>(character);
        if (!(std::isalnum(value) || character == '_')) {
            return false;
        }
    }
    return true;
}

bool is_reserved_command(const std::string_view name) {
    for (const ac::protocol::writer::Command& command :
         ac::protocol::writer::commands) {
        if (command.name == name) {
            return true;
        }
    }
    return false;
}

bool plain_filename(const std::string_view filename) {
    return !filename.empty() &&
        filename != "." &&
        filename != ".." &&
        filename.find('/') == std::string_view::npos &&
        filename.find('\\') == std::string_view::npos;
}

std::optional<std::string> command_name_from_stem(const std::wstring_view stem) {
    std::string body;
    bool pending_underscore = false;
    bool wrote = false;
    for (const wchar_t unit : stem) {
        if (unit < 128) {
            const auto value = static_cast<unsigned char>(unit);
            if (std::isalpha(value) || std::isdigit(value)) {
                if (pending_underscore && wrote) {
                    body.push_back('_');
                }
                pending_underscore = false;
                wrote = true;
                body.push_back(static_cast<char>(std::tolower(value)));
                continue;
            }
        }
        if (wrote) {
            pending_underscore = true;
        }
    }
    if (body.empty()) {
        return std::nullopt;
    }
    return "insert_" + body;
}

std::string utf8_or_placeholder(const std::wstring_view text) {
    try {
        return ac::encoding::to_utf8(text);
    }
    catch (const std::exception&) {
        return "<unprintable filename>";
    }
}

std::optional<std::vector<std::filesystem::path>> list_text_files(
    std::string& failure
) {
    const std::filesystem::path directory = text_directory();
    std::error_code error;
    const bool exists = std::filesystem::exists(directory, error);
    if (error || !exists || !std::filesystem::is_directory(directory, error) ||
        error) {
        failure = "Unable to open text directory: " + directory.string();
        return std::nullopt;
    }

    std::filesystem::directory_iterator cursor {directory, error};
    if (error) {
        failure = "Unable to open text directory: " + directory.string();
        return std::nullopt;
    }

    std::vector<std::filesystem::path> files;
    while (cursor != std::filesystem::directory_iterator {}) {
        std::error_code status_error;
        const std::filesystem::path path = cursor->path();
        const bool regular = cursor->is_regular_file(status_error);
        if (status_error) {
            failure = std::format(
                "Unable to inspect {}: {}",
                path.string(),
                status_error.message()
            );
            return std::nullopt;
        }
        if (regular && extension_is_txt(path)) {
            files.push_back(path);
        }

        cursor.increment(error);
        if (error) {
            failure = "Unable to read text directory: " + directory.string();
            return std::nullopt;
        }
    }

    std::ranges::sort(files, [](
        const std::filesystem::path& left,
        const std::filesystem::path& right
    ) {
        return filename_less(
            left.filename().wstring(),
            right.filename().wstring()
        );
    });
    return files;
}

const TextCommand* find_command(
    const std::span<const TextCommand> commands,
    const std::string_view name
) {
    const auto found = std::ranges::find_if(
        commands,
        [name](const TextCommand& command) { return command.name == name; }
    );
    if (found == commands.end()) {
        return nullptr;
    }
    return &*found;
}

void accept_command(
    CatalogResult& result,
    std::string name,
    std::string filename,
    const std::string_view display_name
) {
    if (!is_command_name(name)) {
        result.issues.push_back(std::format(
            "{} was skipped because it does not produce a command name.",
            display_name
        ));
        return;
    }
    if (is_reserved_command(name)) {
        result.issues.push_back(std::format(
            "{} conflicts with the existing Writer command {}. "
            "It was not added.",
            display_name,
            name
        ));
        return;
    }
    if (const TextCommand* existing = find_command(result.commands, name)) {
        result.issues.push_back(std::format(
            "{} conflicts with {}. Both normalize to {}. Keeping {}.",
            display_name,
            existing->filename,
            name,
            existing->filename
        ));
        return;
    }
    if (!plain_filename(filename)) {
        result.issues.push_back(std::format(
            "{} was skipped because its filename cannot be stored.",
            display_name
        ));
        return;
    }
    result.commands.push_back(TextCommand {
        std::move(name),
        std::move(filename)
    });
}

std::string_view trim_line_ending(std::string_view line) {
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }
    return line;
}

} // namespace

std::filesystem::path text_directory() {
    return ac::paths::writer_directory() / "text";
}

std::filesystem::path catalog_path() {
    return ac::paths::writer_directory() / "insert_text.list";
}

CatalogResult build_catalog() {
    CatalogResult result;
    std::string failure;
    const auto files = list_text_files(failure);
    if (!files) {
        result.unavailable = true;
        result.issues.push_back(std::move(failure));
        return result;
    }

    for (const std::filesystem::path& path : *files) {
        const std::wstring filename = path.filename().wstring();
        const std::string display_name = utf8_or_placeholder(filename);
        std::string stored_name;
        try {
            stored_name = ac::encoding::to_utf8(filename);
        }
        catch (const std::exception&) {
            result.issues.push_back(std::format(
                "{} was skipped because its filename could not be stored.",
                display_name
            ));
            continue;
        }

        const auto command_name = command_name_from_stem(path.stem().wstring());
        if (!command_name) {
            result.issues.push_back(std::format(
                "{} was skipped because it does not produce a command name.",
                display_name
            ));
            continue;
        }
        accept_command(result, *command_name, stored_name, display_name);
    }
    return result;
}

CatalogResult read_catalog() {
    CatalogResult result;
    const std::filesystem::path path = catalog_path();
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        result.unavailable = true;
        result.issues.push_back(
            "Unable to inspect " + path.string() + ": " + error.message()
        );
        return result;
    }
    if (!exists) {
        result.missing = true;
        return result;
    }

    std::ifstream input {path, std::ios::binary};
    if (!input) {
        result.unavailable = true;
        result.issues.push_back("Unable to open " + path.string());
        return result;
    }

    std::string line;
    while (std::getline(input, line)) {
        const std::string_view text = trim_line_ending(line);
        if (text.empty()) {
            continue;
        }
        constexpr std::string_view separator {" = "};
        const auto split = text.find(separator);
        if (split == std::string_view::npos) {
            result.issues.push_back(std::format(
                "Invalid text command line: {}",
                text
            ));
            continue;
        }
        accept_command(
            result,
            std::string {text.substr(0, split)},
            std::string {text.substr(split + separator.size())},
            std::string {text}
        );
    }

    if (input.bad()) {
        result.unavailable = true;
        result.commands.clear();
        result.issues.push_back("Unable to read " + path.string());
    }
    return result;
}

bool write_catalog(const std::span<const TextCommand> commands) {
    const std::filesystem::path path = catalog_path();
    std::string contents;
    for (const TextCommand& command : commands) {
        contents += command.name;
        contents += " = ";
        contents += command.filename;
        contents += '\n';
    }

    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return false;
    }

    const auto temporary = std::filesystem::path {path.wstring() + L".new"};
    {
        std::ofstream output {temporary, std::ios::binary | std::ios::trunc};
        if (!output) {
            return false;
        }
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.flush();
        if (!output) {
            output.close();
            ::DeleteFileW(temporary.c_str());
            return false;
        }
    }

    if (!::MoveFileExW(
            temporary.c_str(),
            path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        )) {
        ::DeleteFileW(temporary.c_str());
        return false;
    }
    return true;
}

std::optional<std::string> read_text_file(const std::filesystem::path& file) {
    std::ifstream input {file, std::ios::binary};
    if (!input) {
        return std::nullopt;
    }

    std::string bytes {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
    if (input.bad()) {
        return std::nullopt;
    }
    return bytes;
}

} // namespace writer::text_commands
