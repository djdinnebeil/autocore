/**
 * \file keymap_commands.cxx
 * \brief Builds the production component keymap command catalogs.
 *
 * Called in-process by auto_core_keymap.exe. This file has no process entry.
 * A catalog line is a command name, or `name - description` when the
 * description is non-empty. The same names, without descriptions, fill the
 * final `[commands]` section of `keymap.map`.
 */
#include "../../runtime/keyboard/keymap_map_detail.hpp"

import std;
import command_registry;

import auto_core.core.paths;

import <Windows.h>;

namespace {

struct Provider {
    std::string_view name;
    std::wstring_view executable;
    std::string_view catalog_filename;
};

constexpr Provider providers[] {
    {"dash", L"dash_ac.exe", "dash.keymap_commands.txt"},
    {"itunes", L"itunes_ac.exe", "itunes.keymap_commands.txt"},
    {"journal", L"journal_ac.exe", "journal.keymap_commands.txt"},
    {"logger", L"logger_ac.exe", "logger.keymap_commands.txt"},
    {"server", L"server_ac.exe", "server.keymap_commands.txt"},
    {"slash", L"slash_ac.exe", "slash.keymap_commands.txt"},
    {"spotify", L"spotify_ac.exe", "spotify.keymap_commands.txt"},
    {"taskbar", L"taskbar_ac.exe", "taskbar.keymap_commands.txt"},
    {"wake", L"wake_ac.exe", "wake.keymap_commands.txt"},
    {"writer", L"writer_ac.exe", "writer.keymap_commands.txt"},
};

struct ExportedCommand {
    std::string name;
    std::string description;
};

struct CapturedExport {
    std::vector<ExportedCommand> commands;
};

[[nodiscard]]
bool is_command_name(const std::string_view name) {
    if (name.empty()) {
        return false;
    }
    const auto first = static_cast<unsigned char>(name.front());
    if (first != '_' && !std::isalpha(first)) {
        return false;
    }
    return std::ranges::all_of(name, [](const char character) {
        const auto value = static_cast<unsigned char>(character);
        return value == '_' || std::isalnum(value);
    });
}

[[nodiscard]]
std::optional<std::vector<ExportedCommand>> parse_export(
    const std::string_view text,
    std::string& failure
) {
    if (text.empty()) {
        return std::vector<ExportedCommand> {};
    }

    constexpr std::string_view separator = " - ";
    std::vector<ExportedCommand> commands;
    std::size_t offset = 0;
    while (offset < text.size()) {
        const std::size_t newline = text.find('\n', offset);
        std::string_view line = text.substr(
            offset,
            newline == std::string_view::npos
                ? std::string_view::npos
                : newline - offset
        );
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        const auto split = line.find(separator);
        const std::string_view name = split == std::string_view::npos
            ? line
            : line.substr(0, split);
        const std::string_view description = split == std::string_view::npos
            ? std::string_view {}
            : line.substr(split + separator.size());
        if (name.empty() || !is_command_name(name)) {
            failure = name.empty()
                ? "export contained an empty line"
                : "export contained an invalid command name";
            return std::nullopt;
        }
        commands.push_back(ExportedCommand {
            std::string {name},
            std::string {description}
        });
        if (newline == std::string_view::npos) {
            break;
        }
        offset = newline + 1;
    }

    std::vector<ExportedCommand> ordered = commands;
    std::ranges::sort(ordered, {}, &ExportedCommand::name);
    if (std::ranges::adjacent_find(
            ordered,
            {},
            &ExportedCommand::name
        ) != ordered.end()) {
        failure = "export contained a duplicate command name";
        return std::nullopt;
    }
    return ordered;
}

struct ProcessHandles {
    HANDLE read_pipe = nullptr;
    HANDLE write_pipe = nullptr;
    HANDLE process = nullptr;
    HANDLE thread = nullptr;

    ~ProcessHandles() {
        if (read_pipe) {
            CloseHandle(read_pipe);
        }
        if (write_pipe) {
            CloseHandle(write_pipe);
        }
        if (process) {
            CloseHandle(process);
        }
        if (thread) {
            CloseHandle(thread);
        }
    }

    ProcessHandles() = default;
    ProcessHandles(const ProcessHandles&) = delete;
    ProcessHandles& operator=(const ProcessHandles&) = delete;
};

[[nodiscard]]
std::optional<std::string> capture_stdout(
    const std::filesystem::path& executable,
    const std::wstring_view arguments,
    DWORD& exit_code,
    std::string& failure
) {
    ProcessHandles handles;
    SECURITY_ATTRIBUTES inheritable {};
    inheritable.nLength = sizeof(inheritable);
    inheritable.bInheritHandle = TRUE;
    if (!CreatePipe(&handles.read_pipe, &handles.write_pipe, &inheritable, 0)) {
        failure = "unable to capture export output";
        return std::nullopt;
    }
    if (!SetHandleInformation(handles.read_pipe, HANDLE_FLAG_INHERIT, 0)) {
        failure = "unable to capture export output";
        return std::nullopt;
    }

    STARTUPINFOW startup {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = handles.write_pipe;
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    std::wstring command = L"\"";
    command += executable.wstring();
    command += L"\" ";
    command += arguments;
    std::vector<wchar_t> command_line {command.begin(), command.end()};
    command_line.push_back(L'\0');

    PROCESS_INFORMATION process {};
    if (!CreateProcessW(
            executable.c_str(),
            command_line.data(),
            nullptr,
            nullptr,
            TRUE,
            0,
            nullptr,
            nullptr,
            &startup,
            &process
        )) {
        failure = "unable to start " + executable.filename().string();
        return std::nullopt;
    }
    handles.process = process.hProcess;
    handles.thread = process.hThread;
    CloseHandle(handles.write_pipe);
    handles.write_pipe = nullptr;

    std::string output;
    char buffer[4096];
    DWORD read = 0;
    while (ReadFile(handles.read_pipe, buffer, sizeof(buffer), &read, nullptr) &&
           read > 0) {
        output.append(buffer, buffer + read);
    }

    if (WaitForSingleObject(handles.process, INFINITE) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(handles.process, &exit_code)) {
        failure = "unable to read export exit code";
        return std::nullopt;
    }
    return output;
}

[[nodiscard]]
std::optional<CapturedExport> export_provider(const Provider& provider) {
    const std::filesystem::path executable =
        ac::paths::component_executable_directory() / provider.executable;
    std::error_code exists_error;
    if (!std::filesystem::is_regular_file(executable, exists_error) ||
        exists_error) {
        std::cerr << provider.name
                  << " export failed: unable to find "
                  << executable.string() << '\n';
        return std::nullopt;
    }

    DWORD exit_code = 1;
    std::string failure;
    const std::wstring_view arguments = L"--run --export-keymap-commands";
    const auto output = capture_stdout(
        executable,
        arguments,
        exit_code,
        failure
    );
    if (!output) {
        std::cerr << provider.name << " export failed: " << failure << '\n';
        return std::nullopt;
    }
    if (exit_code != 0) {
        std::cerr << provider.name << " export failed: exit code "
                  << exit_code << '\n';
        return std::nullopt;
    }

    auto names = parse_export(*output, failure);
    if (!names) {
        std::cerr << provider.name << " export failed: " << failure << '\n';
        return std::nullopt;
    }
    return CapturedExport {std::move(*names)};
}

struct OwnedTemporary {
    HANDLE handle {INVALID_HANDLE_VALUE};
    std::filesystem::path path;

    ~OwnedTemporary() {
        if (handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
    }

    OwnedTemporary() = default;
    OwnedTemporary(const OwnedTemporary&) = delete;
    OwnedTemporary& operator=(const OwnedTemporary&) = delete;
};

void remove_owned_temporary(OwnedTemporary& temporary) {
    if (temporary.handle != INVALID_HANDLE_VALUE) {
        CloseHandle(temporary.handle);
        temporary.handle = INVALID_HANDLE_VALUE;
    }
    if (!temporary.path.empty()) {
        DeleteFileW(temporary.path.c_str());
    }
}

[[nodiscard]]
bool create_catalog_temporary(
    const std::filesystem::path& destination,
    OwnedTemporary& temporary
) {
    const auto directory = destination.parent_path();
    const auto filename = destination.filename().wstring();
    const DWORD pid = GetCurrentProcessId();
    for (unsigned attempt = 0; attempt < 16; ++attempt) {
        temporary.path =
            directory /
            (filename + L"." + std::to_wstring(pid) + L"." +
                std::to_wstring(attempt) + L".tmp");
        temporary.handle = CreateFileW(
            temporary.path.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (temporary.handle != INVALID_HANDLE_VALUE) {
            return true;
        }
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_EXISTS) {
            std::cerr << "Failed to create " << temporary.path.string()
                      << " (error " << error << ")\n";
            temporary.path.clear();
            return false;
        }
    }
    std::cerr << "Failed to create a temporary file for "
              << destination.string() << '\n';
    temporary.path.clear();
    return false;
}

[[nodiscard]]
bool write_catalog_temporary(
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

[[nodiscard]]
bool publish_text(
    const std::filesystem::path& destination,
    const std::string_view contents,
    const bool replace_existing
) {
    OwnedTemporary temporary;
    if (!create_catalog_temporary(destination, temporary)) {
        return false;
    }

    DWORD error = 0;
    if (!write_catalog_temporary(temporary.handle, contents, error)) {
        const auto failed_path = temporary.path;
        remove_owned_temporary(temporary);
        std::cerr << "Failed to write " << failed_path.string()
                  << " (error " << error << ")\n";
        return false;
    }
    if (!CloseHandle(temporary.handle)) {
        temporary.handle = INVALID_HANDLE_VALUE;
        const DWORD close_error = GetLastError();
        const auto failed_path = temporary.path;
        remove_owned_temporary(temporary);
        std::cerr << "Failed to close " << failed_path.string()
                  << " (error " << close_error << ")\n";
        return false;
    }
    temporary.handle = INVALID_HANDLE_VALUE;

    const DWORD flags = replace_existing
        ? MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        : MOVEFILE_WRITE_THROUGH;
    if (!MoveFileExW(temporary.path.c_str(), destination.c_str(), flags)) {
        const DWORD move_error = GetLastError();
        const auto failed_path = temporary.path;
        remove_owned_temporary(temporary);
        if (!replace_existing && move_error == ERROR_ALREADY_EXISTS) {
            std::cerr << destination.string()
                      << " already exists. keymap.map was not changed.\n";
        }
        else if (replace_existing) {
            std::cerr << "Failed to replace " << destination.string()
                      << " from " << failed_path.string() << '\n';
        }
        else {
            std::cerr << "Failed to publish " << destination.string()
                      << " from " << failed_path.string() << '\n';
        }
        return false;
    }
    temporary.path.clear();
    return true;
}

[[nodiscard]]
bool replace_catalog(
    const std::filesystem::path& destination,
    const std::vector<ExportedCommand>& commands
) {
    std::string contents;
    for (const ExportedCommand& command : commands) {
        contents += command_registry::format_catalog_line(
            command.name,
            command.description
        );
        contents += '\n';
    }

    std::error_code directory_error;
    std::filesystem::create_directories(
        destination.parent_path(),
        directory_error
    );
    if (directory_error) {
        std::cerr << "Failed to create " << destination.parent_path().string()
                  << ": " << directory_error.message() << '\n';
        return false;
    }

    return publish_text(destination, contents, true);
}

struct TextLine {
    std::size_t begin = 0;
    std::size_t content_end = 0;
};

[[nodiscard]]
std::vector<TextLine> split_lines(const std::string_view text) {
    std::vector<TextLine> lines;
    std::size_t index = 0;
    while (index < text.size()) {
        TextLine line;
        line.begin = index;
        const auto newline = text.find('\n', index);
        if (newline == std::string_view::npos) {
            line.content_end = text.size();
            lines.push_back(line);
            break;
        }
        line.content_end = newline;
        if (line.content_end > line.begin && text[line.content_end - 1] == '\r') {
            --line.content_end;
        }
        lines.push_back(line);
        index = newline + 1;
    }
    return lines;
}

[[nodiscard]]
std::string_view line_text(
    const std::string_view text,
    const TextLine& line
) {
    return text.substr(line.begin, line.content_end - line.begin);
}

[[nodiscard]]
std::string command_section(
    const std::vector<std::string>& names,
    const std::string_view newline
) {
    std::string section = "[commands]";
    section += newline;
    for (const std::string& name : names) {
        section += name;
        section += newline;
    }
    return section;
}

[[nodiscard]]
std::string default_keymap_map(const std::vector<std::string>& names) {
    std::string contents =
        "# Key binding: key = primary | secondary\n"
        "# Example: numpad_1 = activate_auto_core | close_program\n"
        "# Leave a binding blank to return control to the OS\n"
        "\n"
        "[keymap: concise]\n";
    contents += ac::keymap::map_file::format_line(
        "numpad_0",
        "activate_function_key",
        "deactivate_function_key"
    );
    contents += ac::keymap::map_file::format_line(
        "numpad_1",
        "activate_auto_core",
        "close_program"
    );
    constexpr std::string_view blank_keys[] {
        "numpad_2",
        "numpad_3",
        "numpad_4",
        "numpad_5",
        "numpad_6",
        "numpad_7",
        "numpad_8",
        "numpad_9",
        "numpad_multiply",
        "numpad_add",
        "numpad_decimal",
        "numpad_enter",
        "numpad_subtract",
        "numpad_divide",
        "media_play_pause",
        "browser_home",
        "launch_mail",
        "launch_calculator",
    };
    for (const std::string_view key : blank_keys) {
        contents += ac::keymap::map_file::format_line(key, {}, {});
    }
    ac::keymap::map_file::ensure_blank_line(contents, "\n");
    contents += command_section(names, "\n");
    return contents;
}

[[nodiscard]]
bool compose_keymap_map(
    const std::string& existing,
    const std::vector<std::string>& names,
    std::string& updated
) {
    if (existing.find('\0') != std::string::npos) {
        std::cerr << "keymap.map contains a NUL byte. "
                     "keymap.map was not changed.\n";
        return false;
    }

    const std::string newline =
        existing.find("\r\n") == std::string::npos ? "\n" : "\r\n";
    const auto lines = split_lines(existing);
    std::vector<std::size_t> commands_headers;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        const auto trimmed = ac::keymap::map_file::trim(
            line_text(existing, lines[index])
        );
        if (ac::keymap::map_file::is_commands_header(trimmed)) {
            commands_headers.push_back(index);
        }
    }

    if (commands_headers.size() > 1) {
        std::cerr << "keymap.map contains more than one [commands] section. "
                     "keymap.map was not changed.\n";
        return false;
    }

    std::string prefix;
    if (commands_headers.empty()) {
        prefix = existing;
        if (!prefix.empty() && prefix.back() != '\n') {
            prefix += newline;
        }
    }
    else {
        const std::size_t header = commands_headers.front();
        for (std::size_t index = header + 1; index < lines.size(); ++index) {
            const auto text = line_text(existing, lines[index]);
            const auto trimmed = ac::keymap::map_file::trim(text);
            if (ac::keymap::map_file::is_section_header(trimmed)) {
                std::cerr << "keymap.map has a section after [commands]. "
                             "keymap.map was not changed.\n";
                return false;
            }
            if (ac::keymap::map_file::parse_line(text)) {
                std::cerr << "keymap.map has a binding inside [commands]. "
                             "keymap.map was not changed.\n";
                return false;
            }
        }
        prefix = existing.substr(0, lines[header].begin);
    }

    ac::keymap::map_file::ensure_blank_line(prefix, newline);
    updated = std::move(prefix);
    updated += command_section(names, newline);
    return true;
}

[[nodiscard]]
std::optional<std::string> read_keymap_bytes(
    const std::filesystem::path& path
) {
    std::ifstream input {path, std::ios::binary};
    if (!input) {
        std::cerr << "Failed to read " << path.string()
                  << ". keymap.map was not changed.\n";
        return std::nullopt;
    }
    std::string bytes {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
    if (input.bad()) {
        std::cerr << "Failed to read " << path.string()
                  << ". keymap.map was not changed.\n";
        return std::nullopt;
    }
    return bytes;
}

[[nodiscard]]
bool update_keymap_map(const std::vector<std::string>& names) {
    const auto path = ac::paths::keymap_file();
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        std::cerr << "Failed to inspect " << path.string() << ": "
                  << error.message() << ". keymap.map was not changed.\n";
        return false;
    }
    if (!exists) {
        return publish_text(path, default_keymap_map(names), false);
    }

    error.clear();
    if (std::filesystem::is_directory(path, error) && !error) {
        std::cerr << path.string()
                  << " is a directory. keymap.map was not changed.\n";
        return false;
    }
    if (error) {
        std::cerr << "Failed to inspect " << path.string() << ": "
                  << error.message() << ". keymap.map was not changed.\n";
        return false;
    }
    error.clear();
    if (!std::filesystem::is_regular_file(path, error) || error) {
        if (error) {
            std::cerr << "Failed to inspect " << path.string() << ": "
                      << error.message() << ". keymap.map was not changed.\n";
        }
        else {
            std::cerr << path.string()
                      << " is not a file. keymap.map was not changed.\n";
        }
        return false;
    }

    const auto existing = read_keymap_bytes(path);
    if (!existing) {
        return false;
    }
    std::string updated;
    if (!compose_keymap_map(*existing, names, updated)) {
        return false;
    }
    if (updated == *existing) {
        return true;
    }
    return publish_text(path, updated, true);
}

/**
 * \brief Exclusive build guard for this installation.
 *
 * The lock file is `<installation root>/data/auto_core/keymap_build.lock`.
 * A byte-range lock is visible to every Windows session that can open that
 * path and is released when this process exits. It is not a catalog
 * transaction.
 */
class BuildLock {
public:
    enum class Result {
        acquired,
        busy,
        failed
    };

    BuildLock() = default;
    BuildLock(const BuildLock&) = delete;
    BuildLock& operator=(const BuildLock&) = delete;

    ~BuildLock() {
        release();
    }

    [[nodiscard]]
    Result acquire(const std::filesystem::path& lock_path, std::string& error) {
        handle_ = CreateFileW(
            lock_path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (handle_ == INVALID_HANDLE_VALUE) {
            error =
                "Failed to open " + lock_path.string() +
                " (error " + std::to_string(GetLastError()) + ").";
            return Result::failed;
        }

        OVERLAPPED overlapped {};
        if (!LockFileEx(
                handle_,
                LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                0,
                1,
                0,
                &overlapped
            )) {
            const DWORD lock_error = GetLastError();
            CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
            if (lock_error == ERROR_LOCK_VIOLATION) {
                return Result::busy;
            }
            error =
                "Failed to lock " + lock_path.string() +
                " (error " + std::to_string(lock_error) + ").";
            return Result::failed;
        }
        locked_ = true;
        return Result::acquired;
    }

private:
    void release() noexcept {
        if (locked_ && handle_ != INVALID_HANDLE_VALUE) {
            OVERLAPPED overlapped {};
            UnlockFileEx(handle_, 0, 1, 0, &overlapped);
            locked_ = false;
        }
        if (handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
        }
    }

    HANDLE handle_ {INVALID_HANDLE_VALUE};
    bool locked_ {false};
};

} // namespace

int build_keymap_commands() {
    const auto lock_path =
        ac::paths::installation_root() / "data" / "auto_core" /
        "keymap_build.lock";
    std::error_code directory_error;
    std::filesystem::create_directories(
        lock_path.parent_path(),
        directory_error
    );
    if (directory_error) {
        std::cerr << "Failed to create " << lock_path.parent_path().string()
                  << ": " << directory_error.message() << '\n';
        return 1;
    }

    BuildLock build_lock;
    std::string lock_error;
    const auto lock = build_lock.acquire(lock_path, lock_error);
    if (lock == BuildLock::Result::busy) {
        std::cerr << "A keymap command build is already running.\n";
        return 1;
    }
    if (lock != BuildLock::Result::acquired) {
        std::cerr << lock_error << '\n';
        return 1;
    }

    try {
        std::vector<CapturedExport> exports;
        exports.reserve(std::size(providers));
        for (const Provider& provider : providers) {
            auto exported = export_provider(provider);
            if (!exported) {
                return 1;
            }
            exports.push_back(std::move(*exported));
        }

        const auto components = ac::paths::keymap_directory() / "components";
        for (std::size_t index = 0; index < exports.size(); ++index) {
            const auto destination =
                components / providers[index].catalog_filename;
            if (!replace_catalog(destination, exports[index].commands)) {
                return 1;
            }
        }

        std::map<std::string, std::string> combined_commands;
        for (const CapturedExport& exported : exports) {
            for (const ExportedCommand& command : exported.commands) {
                const auto [existing, inserted] = combined_commands.emplace(
                    command.name,
                    command.description
                );
                if (!inserted &&
                    existing->second.empty() &&
                    !command.description.empty()) {
                    existing->second = command.description;
                }
            }
        }
        std::vector<ExportedCommand> combined;
        std::vector<std::string> combined_names;
        combined.reserve(combined_commands.size());
        combined_names.reserve(combined_commands.size());
        for (const auto& [name, description] : combined_commands) {
            combined.push_back(ExportedCommand {name, description});
            combined_names.push_back(name);
        }
        if (!replace_catalog(ac::paths::keymap_commands_file(), combined)) {
            return 1;
        }
        if (!update_keymap_map(combined_names)) {
            return 1;
        }
    }
    catch (const std::exception& error) {
        std::cerr << "auto_core_keymap.exe failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
