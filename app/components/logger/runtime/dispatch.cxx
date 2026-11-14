/**
 * \file dispatch.cxx
 * \brief Selects Logger's operation before runtime, settings, or config startup.
 */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>

import std;
import auto_core.core.paths;
import component_dispatch;

namespace ac::logger::runtime {
    int run(int argc, char* argv[]);
}

namespace ac::logger::config {
    int run(int argc, char* argv[]);
}

namespace ac::logger::settings {
    int run();
}

namespace {

[[nodiscard]]
int launch_config_init() {
    const auto executable_path =
        ac::paths::component_executable_directory() / "logger_ac.exe";
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
        std::cerr << "Unable to read the exit code from logger_ac.exe.\n";
        return 1;
    }
    CloseHandle(process_info.hProcess);
    return static_cast<int>(exit_code);
}

[[nodiscard]]
int open_direct() {
    const auto path = ac::paths::component_config_file("logger");
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    // A failed existence check matches settings: treat the INI as present.
    if (error || present) {
        return ac::logger::settings::run();
    }

    const int init_code = launch_config_init();
    if (init_code != 0) {
        return init_code;
    }
    return ac::logger::settings::run();
}

} // namespace

int main(int argc, char* argv[]) {
    auto command = ac::component_dispatch::parse(argc, argv);
    switch (command.operation) {
    case ac::component_dispatch::Operation::none:
        return open_direct();
    case ac::component_dispatch::Operation::settings:
        return ac::logger::settings::run();
    case ac::component_dispatch::Operation::config:
        return ac::logger::config::run(
            static_cast<int>(command.arguments.size()),
            command.arguments.data()
        );
    case ac::component_dispatch::Operation::run:
        return ac::logger::runtime::run(
            static_cast<int>(command.arguments.size()),
            command.arguments.data()
        );
    case ac::component_dispatch::Operation::invalid:
        std::cerr
            << "Usage: logger_ac.exe [--run [runtime arguments] | "
            << "--settings | --config [config arguments]]\n";
        return 1;
    }
    return 1;
}
