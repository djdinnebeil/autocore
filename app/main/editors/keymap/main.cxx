/**
 * \file main.cxx
 * \brief Process entry for auto_core_keymap.exe.
 *
 * No arguments seed a missing `keymap.map`, build the command catalogs and
 * `[commands]` section, then open the keymap menu. `--seed` creates a missing
 * `keymap.map`. `--build` publishes the command catalogs and updates the
 * final `[commands]` section. Seed and build remain separate in-process calls.
 */

import std;
import auto_core.core.shell;
import auto_core.main.config_support;

import <iostream>;

int seed_keymap();
int seed_keymap_if_missing();
int build_keymap_commands();

namespace {

namespace cfg = ac::main::config;

void print_usage(std::ostream& output) {
    output << "Usage: auto_core_keymap.exe [--seed | --build | --help]\n";
}

/**
 * \brief Seeds a missing map, then builds catalogs and `[commands]`.
 *
 * `seed_keymap()` does not take the build lock. The build acquires that lock
 * once. This function does not call itself.
 */
[[nodiscard]]
int rebuild_keymap() {
    if (seed_keymap_if_missing() != 0) {
        return 1;
    }
    return build_keymap_commands();
}

[[nodiscard]]
int run_menu() {
    bool refresh = true;
    while (true) {
        if (refresh) {
            std::cout << "Keymap commands\n";
            const int result = rebuild_keymap();
            std::cout << '\n';
            if (result == 0) {
                std::cout << "Keymap commands successfully built.\n";
            }
            else {
                std::cout << "Keymap commands could not be built.\n";
            }
            refresh = false;
        }
        std::cout
            << "\n"
            << "1. Rebuild keymap\n"
            << "2. Exit\n"
            << "\n"
            << "Choice [2]: ";
        const auto line = cfg::read_line();
        if (!line || line->empty() || *line == "2") {
            return 0;
        }
        if (*line == "1") {
            refresh = true;
            continue;
        }
        std::cout << "Enter 1 or 2.\n";
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
