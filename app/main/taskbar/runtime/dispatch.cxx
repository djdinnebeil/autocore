/**
 * \file dispatch.cxx
 * \brief Selects Taskbar's operation before runtime, settings, or a tool.
 */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>

import std;
import auto_core.core.paths;
import component_dispatch;

namespace ac::taskbar_app::runtime {
    int run(int argc, char* argv[]);
}

namespace ac::taskbar_app::config {
    int run(int argc, char* argv[]);
}

namespace ac::taskbar_app::settings {
    int run();
    int run_after_discovery();
}

namespace ac::taskbar_app::builder {
    int run(int argc, char* argv[]);
    int setup_application(std::string_view key);
}

namespace {

[[nodiscard]]
int launch_config_init() {
    const auto executable_path =
        ac::paths::app_directory() / "auto_core_taskbar.exe";
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
        std::cerr << "Unable to read the exit code from auto_core_taskbar.exe.\n";
        return 1;
    }
    CloseHandle(process_info.hProcess);
    return static_cast<int>(exit_code);
}

[[nodiscard]]
int open_direct() {
    const auto path = ac::paths::taskbar_policy_file();
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error || present) {
        return ac::taskbar_app::settings::run_after_discovery();
    }

    const int init_code = launch_config_init();
    if (init_code != 0) {
        return init_code;
    }
    return ac::taskbar_app::settings::run_after_discovery();
}

void print_usage() {
    std::cerr
        << "Usage: auto_core_taskbar.exe [--run [runtime arguments] | --settings | "
        << "--builder [builder arguments] | "
        << "--setup-application <application_key> | "
        << "--config [config arguments]]\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 1 && argv != nullptr && argv[1] != nullptr &&
        std::string_view {argv[1]} == "--builder") {
        std::vector<char*> arguments;
        arguments.reserve(static_cast<std::size_t>(argc - 1));
        arguments.push_back(argv[0]);
        for (int index = 2; index < argc; ++index) {
            arguments.push_back(argv[index]);
        }
        return ac::taskbar_app::builder::run(
            static_cast<int>(arguments.size()),
            arguments.data()
        );
    }

    if (argc > 1 && argv != nullptr && argv[1] != nullptr &&
        std::string_view {argv[1]} == "--setup-application") {
        if (argc != 3 || argv[2] == nullptr) {
            print_usage();
            return 1;
        }
        return ac::taskbar_app::builder::setup_application(argv[2]);
    }

    auto command = ac::component_dispatch::parse(argc, argv);
    switch (command.operation) {
    case ac::component_dispatch::Operation::none:
        return open_direct();
    case ac::component_dispatch::Operation::settings:
        return ac::taskbar_app::settings::run();
    case ac::component_dispatch::Operation::config:
        return ac::taskbar_app::config::run(
            static_cast<int>(command.arguments.size()),
            command.arguments.data()
        );
    case ac::component_dispatch::Operation::run:
        return ac::taskbar_app::runtime::run(
            static_cast<int>(command.arguments.size()),
            command.arguments.data()
        );
    case ac::component_dispatch::Operation::invalid:
        print_usage();
        return 1;
    }
    return 1;
}
