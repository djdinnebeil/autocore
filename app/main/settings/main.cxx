/**
 * \file main.cxx
 * \brief Process entry for auto_core_settings.exe.
 *
 * No arguments open the settings menu. `--config` runs Main configuration.
 * `--initialize` runs first-run initialization. The dispatch token is removed
 * before the existing parser sees the remaining arguments.
 */

#include <iostream>
#include <string_view>
#include <vector>

import auto_core.main.settings_app;

int run_configuration_program(int argc, char* argv[]);
int run_initialization_program(int argc, char* argv[]);

namespace {

[[nodiscard]]
std::vector<char*> arguments_after_dispatch(int argc, char* argv[]) {
    std::vector<char*> arguments;
    arguments.reserve(static_cast<std::size_t>(argc - 1));
    arguments.push_back(argv[0]);
    for (int index = 2; index < argc; ++index) {
        arguments.push_back(argv[index]);
    }
    return arguments;
}

}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return run_settings_application();
    }

    const std::string_view command {argv[1]};
    if (command == "--config") {
        auto arguments = arguments_after_dispatch(argc, argv);
        return run_configuration_program(
            static_cast<int>(arguments.size()),
            arguments.data()
        );
    }
    if (command == "--initialize") {
        auto arguments = arguments_after_dispatch(argc, argv);
        return run_initialization_program(
            static_cast<int>(arguments.size()),
            arguments.data()
        );
    }

    std::cerr
        << "Unknown argument: " << argv[1] << "\n"
        << "Usage: auto_core_settings.exe "
           "[--config [configuration arguments] | "
           "--initialize [initialization arguments]]\n";
    return 1;
}
