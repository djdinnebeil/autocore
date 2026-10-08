/**
 * \file main.cxx
 * \brief Builds the production component keymap command catalogs.
 */
import std;

import auto_core.core.paths;
import auto_core.core.shell;

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
    command += L"\" --export-keymap-commands";
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
        ac::paths::bin_directory() / provider.executable;
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
    const auto output = capture_stdout(executable, exit_code, failure);
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

    const std::filesystem::path temporary = destination.wstring() + L".tmp";
    {
        std::ofstream output {temporary, std::ios::binary | std::ios::trunc};
        if (!output) {
            std::cerr << "Failed to create " << temporary.string() << '\n';
            return false;
        }
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.flush();
        if (!output) {
            output.close();
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            std::cerr << "Failed to write " << temporary.string() << '\n';
            return false;
        }
    }

    if (!MoveFileExW(
            temporary.c_str(),
            destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        )) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        std::cerr << "Failed to replace " << destination.string() << '\n';
        return false;
    }
    return true;
}

int build_keymap_commands() {
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
        std::cerr << "keymap_builder.exe failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    const bool build = argc == 1 ||
        (argc == 2 &&
         std::string_view {argv[1]} == "--build-keymap-commands");
    if (!build) {
        std::cerr << "Usage:\n"
                  << "  keymap_builder.exe\n"
                  << "  keymap_builder.exe --build-keymap-commands\n";
        return 1;
    }
    return build_keymap_commands();
}
