#include "../../runtime/keyboard/keymap_map_detail.hpp"

import std;
import auto_core.core.component;
import auto_core.core.paths;
import auto_core.main.key_codes;

import <Windows.h>;

namespace {

ac::Component keymap_editor {"keymap_editor", "auto_core"};

[[nodiscard]]
std::string mappings_seed() {
    std::string contents =
        "# Key binding: key = primary | secondary\n"
        "# Example: numpad_1 = activate_auto_core | close_program\n"
        "# Leave a binding blank to return control to the OS\n"
        "\n"
        "[keymap: concise]\n";
    for (const auto& key : key_codes::keys) {
        if (key.name == "numpad_0") {
            contents += ac::keymap::map_file::format_line(
                key.name,
                "activate_function_key",
                "deactivate_function_key"
            );
        }
        else if (key.name == "numpad_1") {
            contents += ac::keymap::map_file::format_line(
                key.name,
                "activate_auto_core",
                "close_program"
            );
        }
        else {
            contents += ac::keymap::map_file::format_line(key.name, {}, {});
        }
    }
    ac::keymap::map_file::ensure_blank_line(contents, "\n");
    contents += "[commands]\n";
    return contents;
}

struct NewFile {
    HANDLE handle {INVALID_HANDLE_VALUE};

    ~NewFile() {
        if (handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
    }

    NewFile() = default;
    NewFile(const NewFile&) = delete;
    NewFile& operator=(const NewFile&) = delete;
};

[[nodiscard]]
int reject_existing_destination(
    const std::filesystem::path& path,
    const DWORD create_error
) {
    std::error_code error;
    if (std::filesystem::is_regular_file(path, error) && !error) {
        return 0;
    }
    if (error) {
        keymap_editor.print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }

    error.clear();
    if (std::filesystem::is_directory(path, error) && !error) {
        keymap_editor.print(
            "{} is a directory. keymap.map was not written.",
            path.string()
        );
        return 1;
    }
    if (error) {
        keymap_editor.print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }
    if (std::filesystem::exists(path, error) && !error) {
        keymap_editor.print(
            "{} is not a file. keymap.map was not written.",
            path.string()
        );
        return 1;
    }
    keymap_editor.print(
        "Failed to create {} (error {}). keymap.map was not written.",
        path.string(),
        create_error
    );
    return 1;
}

[[nodiscard]]
bool write_new_file(
    const HANDLE handle,
    const std::string_view contents,
    DWORD& error
) {
    std::size_t written_total = 0;
    while (written_total < contents.size()) {
        const auto chunk = static_cast<DWORD>(std::min<std::size_t>(
            contents.size() - written_total,
            static_cast<std::size_t>(1u << 20)
        ));
        DWORD written = 0;
        if (!WriteFile(
                handle,
                contents.data() + written_total,
                chunk,
                &written,
                nullptr
            ) ||
            written != chunk) {
            error = GetLastError();
            if (error == 0) {
                error = ERROR_WRITE_FAULT;
            }
            return false;
        }
        written_total += written;
    }
    if (!FlushFileBuffers(handle)) {
        error = GetLastError();
        if (error == 0) {
            error = ERROR_WRITE_FAULT;
        }
        return false;
    }
    return true;
}

void remove_partial(
    const std::filesystem::path& path,
    std::string& detail
) {
    if (DeleteFileW(path.c_str()) != 0) {
        return;
    }
    detail =
        " The partial file could not be removed (error " +
        std::to_string(GetLastError()) + ").";
}

} // namespace

int seed_keymap() {
    keymap_editor.log_info("auto_core_keymap.exe --seed started");

    const auto path = ac::paths::keymap_file();
    const std::string contents = mappings_seed();
    NewFile created;
    created.handle = CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (created.handle == INVALID_HANDLE_VALUE) {
        return reject_existing_destination(path, GetLastError());
    }

    DWORD error = 0;
    if (!write_new_file(created.handle, contents, error)) {
        CloseHandle(created.handle);
        created.handle = INVALID_HANDLE_VALUE;
        std::string detail;
        remove_partial(path, detail);
        keymap_editor.print(
            "Failed to write {} (error {}). keymap.map was not written.{}",
            path.string(),
            error,
            detail
        );
        return 1;
    }
    if (!CloseHandle(created.handle)) {
        created.handle = INVALID_HANDLE_VALUE;
        const DWORD close_error = GetLastError();
        std::string detail;
        remove_partial(path, detail);
        keymap_editor.print(
            "Failed to close {} (error {}). keymap.map was not written.{}",
            path.string(),
            close_error,
            detail
        );
        return 1;
    }
    created.handle = INVALID_HANDLE_VALUE;
    keymap_editor.log_debug("Wrote {}", path.string());
    return 0;
}

int seed_keymap_if_missing() {
    const auto path = ac::paths::keymap_file();
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        keymap_editor.print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }
    if (exists) {
        return 0;
    }
    return seed_keymap();
}
