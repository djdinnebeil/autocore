/**
 * \file main.cxx
 * \brief Process entry for auto_core_keymap.exe.
 *
 * No arguments open the keymap menu. `--seed` creates a missing
 * `keymap.map`. `--build` publishes the command catalogs. The two
 * operations are separate in-process calls.
 */

import std;
import auto_core.core.shell;
import auto_core.main.config_support;

import <iostream>;

int seed_keymap();
int build_keymap_commands();

namespace {

namespace cfg = ac::main::config;

void print_usage(std::ostream& output) {
    output << "Usage: auto_core_keymap.exe [--seed | --build | --help]\n";
}

[[nodiscard]]
int run_menu() {
    while (true) {
        std::cout
            << '\n'
            << "Keymap\n"
            << "\n"
            << "1. Create keymap if missing\n"
            << "2. Build command list\n"
            << "3. Exit\n"
            << "\n"
            << "Choice [3]: ";
        const auto line = cfg::read_line();
        if (!line || line->empty() || *line == "3") {
            return 0;
        }
        if (*line == "1") {
            return seed_keymap();
        }
        if (*line == "2") {
            return build_keymap_commands();
        }
        std::cout << "Enter 1, 2, or 3.\n";
    }
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    if (argc < 2) {
        return run_menu();
    }
    if (argc > 2) {
        std::cerr << "Unknown argument: " << argv[2] << '\n';
        print_usage(std::cerr);
        return 1;
    }

    const std::string_view command {argv[1]};
    if (command == "--seed") {
        return seed_keymap();
    }
    if (command == "--build") {
        return build_keymap_commands();
    }
    if (command == "--help") {
        print_usage(std::cout);
        return 0;
    }

    std::cerr << "Unknown argument: " << argv[1] << '\n';
    print_usage(std::cerr);
    return 1;
}
