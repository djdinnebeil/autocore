/**
 * \file dispatch.cxx
 * \brief Selects Dash's operation before runtime, settings, editor, or config.
 *
 * The process entry is `wmain` so `--target` and `--parent-pid` keep the
 * wide characters Windows supplied. Configuration flags are ASCII and are
 * passed to the existing parser as UTF-8.
 */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <string>
#include <vector>

import std;
import auto_core.core.paths;

namespace ac::dash_app::runtime {
    int run(int argc, wchar_t* argv[]);
}

namespace ac::dash_app::config {
    int run(int argc, char* argv[]);
}

namespace ac::dash_app::settings {
    int run();
}

namespace ac::dash_app::editor {
    int run();
}

namespace {

[[nodiscard]]
std::optional<std::string> utf8_from_wide(const std::wstring_view value) {
    if (value.empty()) {
        return std::string {};
    }
    const int size = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if (size <= 0) {
        return std::nullopt;
    }
    std::string encoded(static_cast<std::size_t>(size), '\0');
    const int written = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        encoded.data(),
        size,
        nullptr,
        nullptr
    );
    if (written != size) {
        return std::nullopt;
    }
    return encoded;
}

[[nodiscard]]
int launch_config_init() {
    const auto executable_path =
        ac::paths::component_executable_directory() / "dash_ac.exe";
    std::error_code error;
    const bool present = std::filesystem::exists(executable_path, error);
    if (error || !present) {
        std::cerr << "Missing " << executable_path.string() << ".\n";
        return 1;
    }

    std::wstring command_line = L"\"";
    command_line += executable_path.wstring();
    command_line += L"\" \"--config\" \"--init\"";

    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info {};
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
        std::cerr << "Unable to start " << executable_path.string() << ".\n";
        return 1;
    }

    CloseHandle(process_info.hThread);
    WaitForSingleObject(process_info.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        CloseHandle(process_info.hProcess);
        std::cerr << "Unable to read the exit code from dash_ac.exe.\n";
        return 1;
    }
    CloseHandle(process_info.hProcess);
    return static_cast<int>(exit_code);
}

[[nodiscard]]
int open_direct() {
    const auto path = ac::paths::component_config_file("dash");
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    // A failed existence check matches settings: treat the INI as present.
    if (error || present) {
        return ac::dash_app::settings::run();
    }

    const int init_code = launch_config_init();
    if (init_code != 0) {
        return init_code;
    }
    return ac::dash_app::settings::run();
}

[[nodiscard]]
int run_config(const int argc, wchar_t* argv[]) {
    std::vector<std::string> storage;
    storage.reserve(static_cast<std::size_t>(argc - 1));
    for (int index = 0; index < argc; ++index) {
        if (index == 1) {
            continue;
        }
        auto encoded = utf8_from_wide(argv[index] == nullptr
            ? std::wstring_view {}
            : std::wstring_view {argv[index]});
        if (!encoded) {
            std::cerr << "Unable to read a Dash configuration argument.\n";
            return 1;
        }
        storage.push_back(std::move(*encoded));
    }

    std::vector<char*> arguments;
    arguments.reserve(storage.size());
    for (auto& argument : storage) {
        arguments.push_back(argument.data());
    }
    return ac::dash_app::config::run(
        static_cast<int>(arguments.size()),
        arguments.data()
    );
}

void print_usage() {
    std::cerr
        << "Usage: dash_ac.exe [--run [runtime arguments] | --settings | "
        << "--editor | --config [config arguments]]\n";
}

} // namespace

int wmain(int argc, wchar_t* argv[]) {
    if (argc <= 1 || argv == nullptr || argv[1] == nullptr) {
        return open_direct();
    }

    const std::wstring_view operation {argv[1]};
    if (operation == L"--settings") {
        return ac::dash_app::settings::run();
    }
    if (operation == L"--editor") {
        return ac::dash_app::editor::run();
    }
    if (operation == L"--config") {
        return run_config(argc, argv);
    }
    if (operation == L"--run") {
        std::vector<wchar_t*> arguments;
        arguments.reserve(static_cast<std::size_t>(argc - 1));
        arguments.push_back(argv[0]);
        for (int index = 2; index < argc; ++index) {
            arguments.push_back(argv[index]);
        }
        return ac::dash_app::runtime::run(
            static_cast<int>(arguments.size()),
            arguments.data()
        );
    }

    print_usage();
    return 1;
}
