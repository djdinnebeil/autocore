/**
 * \file main.cxx
 * \brief Process entry for auto_core_components.exe.
 *
 * No arguments open the component menu. `--edit` synchronizes
 * `components.list`. `--enable` and `--disable` set one listed state.
 * `--initialize` runs component initialization. `--init` is an alias of
 * `--initialize`. The dispatch token is removed before the role parser
 * sees the remaining arguments.
 */

#include "components_usage.hpp"

import std;
import auto_core.core.shell;
import auto_core.main.config_support;

import <iostream>;

int run_components_editor(int argc, char* argv[]);
int set_component_listed_state(std::string_view name, bool enabled);
int run_components_initialization(int argc, char* argv[]);

void print_components_usage() {
    std::cerr
        << "Usage: auto_core_components.exe "
           "[--edit [--seed] | --enable <name> | --disable <name> | "
           "--initialize [--seed] | --init [--seed]]\n";
}

namespace {

namespace cfg = ac::main::config;

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

[[nodiscard]]
int run_with_program_name(int (*operation)(int, char**)) {
    char program[] = "auto_core_components.exe";
    char* arguments[] = {program};
    return operation(1, arguments);
}

[[nodiscard]]
int run_menu() {
    while (true) {
        std::cout
            << '\n'
            << "Components\n"
            << "\n"
            << "1. Synchronize component list\n"
            << "2. Initialize components\n"
            << "3. Exit\n"
            << "\n"
            << "Choice [3]: ";
        const auto line = cfg::read_line();
        if (!line || line->empty() || *line == "3") {
            return 0;
        }
        if (*line == "1") {
            return run_with_program_name(run_components_editor);
        }
        if (*line == "2") {
            return run_with_program_name(run_components_initialization);
        }
        std::cout << "Enter 1, 2, or 3.\n";
    }
}

[[nodiscard]]
int run_listed_state(const int argc, char* argv[], const bool enabled) {
    const std::string_view command {argv[1]};
    if (argc < 3 || argv[2][0] == '\0') {
        std::cerr << "Missing component name after " << command << ".\n";
        print_components_usage();
        return 1;
    }
    if (argc > 3) {
        std::cerr << "Unknown argument: " << argv[3] << '\n';
        print_components_usage();
        return 1;
    }
    return set_component_listed_state(argv[2], enabled);
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    if (argc < 2) {
        return run_menu();
    }

    const std::string_view command {argv[1]};
    if (command == "--edit") {
        auto arguments = arguments_after_dispatch(argc, argv);
        return run_components_editor(
            static_cast<int>(arguments.size()),
            arguments.data()
        );
    }
    if (command == "--enable") {
        return run_listed_state(argc, argv, true);
    }
    if (command == "--disable") {
        return run_listed_state(argc, argv, false);
    }
    if (command == "--initialize" || command == "--init") {
        auto arguments = arguments_after_dispatch(argc, argv);
        return run_components_initialization(
            static_cast<int>(arguments.size()),
            arguments.data()
        );
    }

    std::cerr << "Unknown argument: " << argv[1] << '\n';
    print_components_usage();
    return 1;
}
