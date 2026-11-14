/**
 * \file keymap_commands.cxx
 * \brief Builds the production component keymap command catalogs.
 *
 * Called in-process by auto_core_keymap.exe. This file has no process entry.
 */
import std;

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

struct CapturedExport {
    std::string provider;
    std::vector<std::string> names;
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
std::optional<std::vector<std::string>> parse_export(
    const std::string_view text,
    std::string& failure
) {
    if (text.empty()) {
        return std::vector<std::string> {};
    }

    std::vector<std::string> names;
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
        if (line.empty() || !is_command_name(line)) {
            failure = line.empty()
                ? "export contained an empty line"
                : "export contained an invalid command name";
            return std::nullopt;
        }
        names.emplace_back(line);
        if (newline == std::string_view::npos) {
            break;
        }
        offset = newline + 1;
    }

    std::vector<std::string> ordered = names;
    std::ranges::sort(ordered);
    if (std::ranges::adjacent_find(ordered) != ordered.end()) {
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
    return CapturedExport {std::string {provider.name}, std::move(*names)};
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
bool replace_catalog(
    const std::filesystem::path& destination,
    const std::vector<std::string>& names
) {
    std::string contents;
    for (const std::string& name : names) {
        contents += name;
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

    if (!MoveFileExW(
            temporary.path.c_str(),
            destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        )) {
        const auto failed_path = temporary.path;
        remove_owned_temporary(temporary);
        std::cerr << "Failed to replace " << destination.string()
                  << " from " << failed_path.string() << '\n';
        return false;
    }
    temporary.path.clear();
    return true;
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
            if (!replace_catalog(destination, exports[index].names)) {
                return 1;
            }
        }

        std::map<std::string, std::vector<std::string>> providers_by_name;
        for (const CapturedExport& exported : exports) {
            for (const std::string& name : exported.names) {
                providers_by_name[name].push_back(exported.provider);
            }
        }
        std::vector<std::string> combined;
        combined.reserve(providers_by_name.size());
        for (const auto& [name, sources] : providers_by_name) {
            if (sources.size() > 1) {
                std::cerr << name << '\n';
                for (const std::string& source : sources) {
                    std::cerr << "    " << source << '\n';
                }
            }
            combined.push_back(name);
        }
        if (!replace_catalog(ac::paths::keymap_commands_file(), combined)) {
            return 1;
        }
    }
    catch (const std::exception& error) {
        std::cerr << "auto_core_keymap.exe failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
