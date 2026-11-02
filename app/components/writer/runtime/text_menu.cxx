module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

module writer_commands;

import std;
import auto_core.core.console;
import auto_core.core.encoding;
import auto_core.core.paths;
import auto_core.core.thread;
import writer_component;

namespace {

const std::filesystem::path& text_directory() {
    static const std::filesystem::path path =
        ac::paths::writer_directory() / "text";
    return path;
}

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

std::optional<std::vector<std::filesystem::path>> list_text_files() {
    const std::filesystem::path& directory = text_directory();
    std::error_code error;
    const bool exists = std::filesystem::exists(directory, error);
    if (error || !exists || !std::filesystem::is_directory(directory, error) ||
        error) {
        writer_component().log_alert(
            "Unable to open text directory: {}",
            directory.string()
        );
        return std::nullopt;
    }

    std::filesystem::directory_iterator cursor {directory, error};
    if (error) {
        writer_component().log_alert(
            "Unable to open text directory: {}",
            directory.string()
        );
        return std::nullopt;
    }

    std::vector<std::filesystem::path> files;
    while (cursor != std::filesystem::directory_iterator {}) {
        std::error_code status_error;
        const std::filesystem::path path = cursor->path();
        const bool regular = cursor->is_regular_file(status_error);
        if (status_error) {
            writer_component().log_alert(
                "Unable to inspect {}: {}",
                path.string(),
                status_error.message()
            );
        }
        else if (regular && extension_is_txt(path)) {
            files.push_back(path);
        }

        cursor.increment(error);
        if (error) {
            writer_component().log_alert(
                "Unable to read text directory: {}",
                directory.string()
            );
            return std::nullopt;
        }
    }

    if (files.empty()) {
        writer_component().log_alert(
            "Text directory contains no text files: {}",
            directory.string()
        );
        return std::nullopt;
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

std::optional<std::filesystem::path> select_text_file(
    const std::vector<std::filesystem::path>& files
) {
    std::ostringstream menu;
    menu << "Select a text file:\n";
    for (std::size_t index = 0; index < files.size(); ++index) {
        menu << std::format(
            "{}. {}\n",
            index + 1,
            ac::encoding::to_utf8(files[index].filename().wstring())
        );
    }
    menu << "> ";
    writer_component().printnl(menu.str());

    std::string input;
    while (std::getline(std::cin, input)) {
        writer_component().log_info("Text file selection: {}", input);
        std::size_t selection = 0;
        const char* const first = input.data();
        const char* const last = first + input.size();
        const auto result = std::from_chars(first, last, selection);
        if (result.ec == std::errc {} && result.ptr == last &&
            selection > 0 && selection <= files.size()) {
            return files[selection - 1];
        }
        writer_component().printnl("Incorrect input\nEnter again: ");
    }

    writer_component().log_alert(
        "Text file selection cancelled: input closed"
    );
    return std::nullopt;
}

std::optional<std::string> read_text_file(const std::filesystem::path& file) {
    std::ifstream input {file, std::ios::binary};
    if (!input) {
        writer_component().log_alert(
            "Unable to open text file: {}",
            file.string()
        );
        return std::nullopt;
    }

    std::string bytes {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
    if (input.bad()) {
        writer_component().log_alert(
            "Unable to read text file: {}",
            file.string()
        );
        return std::nullopt;
    }
    return bytes;
}

void select_and_insert_text_worker() {
    const auto target_window = ac::console::focus_for_prompt_via_winkey();
    if (!target_window) {
        writer_component().log_alert(
            ac::console::error_message(target_window.error())
        );
        return;
    }

    const auto files = list_text_files();
    if (!files) {
        return;
    }
    const auto file = select_text_file(*files);
    if (!file) {
        return;
    }

    const auto bytes = read_text_file(*file);
    if (!bytes) {
        return;
    }

    std::wstring text;
    try {
        text = ac::encoding::to_utf16(*bytes);
    }
    catch (const std::exception& exception) {
        writer_component().log_alert(
            "Unable to convert text for insertion: {}",
            exception.what()
        );
        return;
    }

    writer_component().printnl(*bytes);
    writer_component().insert_text_preserving_clipboard_text(
        *target_window,
        text,
        false
    );
}

} // namespace

void writer_actions::select_and_insert_text() {
    writer_component().log_info("select_and_insert_text()");

    auto lease = SelectionLease::try_acquire();
    if (!lease) {
        writer_component().log_alert(
            "A Writer selection menu is already active"
        );
        return;
    }

    std::thread worker([lease = std::move(*lease)]() mutable {
        ac::thread::run_with_exception_handling(
            select_and_insert_text_worker,
            writer_component()
        );
        (void)lease;
    });
    worker.detach();
}
