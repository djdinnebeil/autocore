/**
* \file main.cxx
* \brief Generates the Auto Core keymap command registry.
*
* The dash component reads the configured project paths, scans for functions
* marked with \keymap_command, generates dash_x.ixx, copies the generated module
* into the keymap directory, creates the required hard link when needed, and
* optionally rebuilds Auto Core.
*/
import std;

import <cstdlib>;

#include "dash_x.hpp"

int main(int argc, char* argv[]) {

    std::cout << "Does this show";
    set_project_paths();

    if (!generate_keymap_command_registry_module()) {
        return 1;
    }

    copy_keymap_command_registry_module();
    create_hard_link();

    if (argc < 2) {
        std::cout << "Finished building dash_x" << std::endl;
    }
    else {
        std::cout << "\n\nBuild Auto Core? Press any key to continue" << std::endl;
        std::cin.get();

        int result = system(build_project_cmd.c_str());

        if (result != 0) {
            std::cerr << "Build failed with a return code of " << result << std::endl;
        }
    }

    std::cout << "Press any key to exit." << std::endl;
    std::cin.get();
    return 0;
}
