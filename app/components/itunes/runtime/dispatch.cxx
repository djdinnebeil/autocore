/**
 * \file dispatch.cxx
 * \brief Selects iTunes' operation before runtime, settings, or a tool.
 */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>

import std;
import auto_core.core.paths;
import auto_core.core.shell;
import component_dispatch;
import itunes_formatter_main;

namespace ac::itunes_app::runtime {
    int run(int argc, char* argv[]);
}

namespace ac::itunes_app::config {
    int run(int argc, char* argv[]);
}

namespace ac::itunes_app::settings {
    int run();
}

namespace {

[[nodiscard]]
int launch_config_init() {
    const auto executable_path =
        ac::paths::component_executable_directory() / "itunes_ac.exe";
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
        std::cerr << "Unable to read the exit code from itunes_ac.exe.\n";
        return 1;
    }
    CloseHandle(process_info.hProcess);
    return static_cast<int>(exit_code);
}

[[nodiscard]]
int open_direct() {
    const auto path = ac::paths::component_config_file("itunes");
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error || present) {
        return ac::itunes_app::settings::run();
    }

    const int init_code = launch_config_init();
    if (init_code != 0) {
        return init_code;
    }
    return ac::itunes_app::settings::run();
}

void print_usage() {
    std::cerr
        << "Usage: itunes_ac.exe [--run [runtime arguments] | --settings | "
        << "--formatter [formatter arguments] | --config [config arguments]]\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 1 && argv != nullptr && argv[1] != nullptr &&
        std::string_view {argv[1]} == "--formatter") {
        std::vector<char*> arguments;
        arguments.reserve(static_cast<std::size_t>(argc - 1));
        arguments.push_back(argv[0]);
        for (int index = 2; index < argc; ++index) {
            arguments.push_back(argv[index]);
        }
        ac::shell::set_process_app_user_model_id();
        return run_itunes_formatter(
            static_cast<int>(arguments.size()),
            arguments.data()
        );
    }

    auto command = ac::component_dispatch::parse(argc, argv);
    switch (command.operation) {
    case ac::component_dispatch::Operation::none:
        return open_direct();
    case ac::component_dispatch::Operation::settings:
        return ac::itunes_app::settings::run();
    case ac::component_dispatch::Operation::config:
        return ac::itunes_app::config::run(
            static_cast<int>(command.arguments.size()),
            command.arguments.data()
        );
    case ac::component_dispatch::Operation::run:
        return ac::itunes_app::runtime::run(
            static_cast<int>(command.arguments.size()),
            command.arguments.data()
        );
    case ac::component_dispatch::Operation::invalid:
        print_usage();
        return 1;
    }
    return 1;
}
