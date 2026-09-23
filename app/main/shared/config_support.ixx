/**
 * \file config_support.ixx
 * \brief Shared helpers for Main `_config.exe` programs.
 */
export module auto_core.main.config_support;

import std;
import auto_core.core.paths;

import <Windows.h>;
import <iostream>;

export namespace ac::main::config {

    [[nodiscard]]
    inline std::string_view trim(const std::string_view value) {
        const auto first = value.find_first_not_of(" \t");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t");
        return value.substr(first, last - first + 1);
    }

    [[nodiscard]]
    inline bool write_bytes(
        const std::filesystem::path& path,
        const std::string_view contents
    ) {
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            std::cerr
                << "Failed to create "
                << path.parent_path().string()
                << ": "
                << error.message()
                << '\n';
            return false;
        }
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            std::cerr << "Failed to create " << path.string() << '\n';
            return false;
        }
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.close();
        if (!output) {
            std::cerr << "Failed to write " << path.string() << '\n';
            return false;
        }
        return true;
    }

    [[nodiscard]]
    inline bool ensure_directory(const std::filesystem::path& directory) {
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        if (error) {
            std::cerr
                << "Failed to create "
                << directory.string()
                << ": "
                << error.message()
                << '\n';
            return false;
        }
        return true;
    }

    [[nodiscard]]
    inline std::optional<std::string> read_line() {
        std::string line;
        if (!std::getline(std::cin, line)) {
            return std::nullopt;
        }
        return std::string {trim(line)};
    }

    [[nodiscard]]
    inline std::optional<std::string> prompt_text(
        const std::string_view label,
        const std::string_view suggestion
    ) {
        std::cout << label << " [" << suggestion << "]: ";
        const auto line = read_line();
        if (!line) {
            return std::nullopt;
        }
        if (line->empty()) {
            return std::string {suggestion};
        }
        return *line;
    }

    [[nodiscard]]
    inline std::optional<std::string> prompt_choice(
        const std::string_view label,
        const std::string_view suggestion,
        const std::initializer_list<std::string_view> allowed
    ) {
        const auto line = prompt_text(label, suggestion);
        if (!line) {
            return std::nullopt;
        }
        std::string lowered {*line};
        for (char& character : lowered) {
            if (character >= 'A' && character <= 'Z') {
                character = static_cast<char>(character - 'A' + 'a');
            }
        }
        for (const auto option : allowed) {
            if (lowered == option) {
                return std::string {option};
            }
        }
        std::cout
            << "Enter";
        bool first = true;
        for (const auto option : allowed) {
            if (first) {
                std::cout << ' ' << option;
                first = false;
            }
            else {
                std::cout << " or " << option;
            }
        }
        std::cout
            << ". Using "
            << *line
            << " is invalid; keeping "
            << suggestion
            << ".\n";
        return std::string {suggestion};
    }

    [[nodiscard]]
    inline std::optional<bool> prompt_on_off(
        const std::string_view label,
        const bool suggestion
    ) {
        const auto line = prompt_choice(
            label,
            suggestion ? "on" : "off",
            {"on", "off"}
        );
        if (!line) {
            return std::nullopt;
        }
        return *line == "on";
    }

    [[nodiscard]]
    inline std::optional<bool> prompt_bool(
        const std::string_view label,
        const bool suggestion
    ) {
        const auto line = prompt_text(label, suggestion ? "true" : "false");
        if (!line) {
            return std::nullopt;
        }
        if (*line == "true" || *line == "on") {
            return true;
        }
        if (*line == "false" || *line == "off") {
            return false;
        }
        std::cout << "Enter true or false. Using " << *line << " is invalid; "
                     "keeping "
                  << (suggestion ? "true" : "false") << ".\n";
        return suggestion;
    }

    [[nodiscard]]
    inline int run_and_wait(
        const std::filesystem::path& executable_path,
        const std::wstring& extra_arguments = {}
    ) {
        STARTUPINFOW startup_info {};
        startup_info.cb = sizeof(startup_info);
        PROCESS_INFORMATION process_info {};

        std::wstring command_line =
            L"\"" + executable_path.wstring() + L"\"";
        if (!extra_arguments.empty()) {
            command_line += L' ';
            command_line += extra_arguments;
        }

        if (!CreateProcessW(
                executable_path.c_str(),
                command_line.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                executable_path.parent_path().c_str(),
                &startup_info,
                &process_info
            )) {
            std::cerr
                << "Unable to start "
                << executable_path.string()
                << '\n';
            return 1;
        }

        CloseHandle(process_info.hThread);
        WaitForSingleObject(process_info.hProcess, INFINITE);
        DWORD exit_code = 1;
        if (!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
            CloseHandle(process_info.hProcess);
            return 1;
        }
        CloseHandle(process_info.hProcess);
        return static_cast<int>(exit_code);
    }

    [[nodiscard]]
    inline int run_config_exe(
        const std::string_view name,
        const std::wstring extra_arguments = {}
    ) {
        return run_and_wait(ac::paths::bin_directory() / name, extra_arguments);
    }

} // namespace ac::main::config
