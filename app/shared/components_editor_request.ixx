/**
 * \file components_editor_request.ixx
 * \brief Launch `components_editor.exe --component <name>` from a child
 *        `_config.exe`. Request identity is `component = <name>`.
 */
export module components_editor_request;

import std;
import auto_core.core.paths;

import <Windows.h>;
import <iostream>;

export namespace ac::config::components_request {

    inline constexpr std::string_view executable_name = "components_editor.exe";
    inline constexpr std::string_view flag = "--component";
    inline constexpr std::string_view initialize_flag = "--initialize";

    [[nodiscard]]
    inline bool is_initialize_run(const int argc, char* argv[]) noexcept {
        for (int i = 1; i < argc; ++i) {
            if (std::string_view {argv[i]} == initialize_flag) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    inline bool is_initialize_run(const int argc, wchar_t* argv[]) noexcept {
        for (int i = 1; i < argc; ++i) {
            if (std::wstring_view {argv[i]} == L"--initialize") {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    inline bool is_valid_component_name(const std::string_view name) noexcept {
        if (name.empty() || name.front() < 'a' || name.front() > 'z') {
            return false;
        }
        for (const char character : name) {
            const bool letter = character >= 'a' && character <= 'z';
            const bool digit = character >= '0' && character <= '9';
            if (!letter && !digit && character != '_') {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]]
    inline int run_component_update(const std::string_view name) {
        if (!is_valid_component_name(name)) {
            std::cerr
                << "Invalid component name for components.list: "
                << name
                << '\n';
            return 1;
        }

        const auto executable_path =
            ac::paths::bin_directory() / executable_name;
        STARTUPINFOW startup_info {};
        startup_info.cb = sizeof(startup_info);
        PROCESS_INFORMATION process_info {};

        std::wstring command_line = L"\"" + executable_path.wstring() + L"\" ";
        command_line.append(flag.begin(), flag.end());
        command_line += L' ';
        command_line.append(name.begin(), name.end());

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
            std::cerr << "Unable to start " << executable_path.string() << '\n';
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

} // namespace ac::config::components_request
