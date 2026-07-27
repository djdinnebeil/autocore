/**
\file main.cxx
\brief Generates the dash_x.ixx runtime configuration module for Auto Core.

The dash component reads its configured project paths, regenerates the runtime
function map, copies the generated module into the local keymap directory,
creates the keymap hard link when needed, and optionally rebuilds Auto Core.
*/
import std;
import logger;
import print;
import <cstdlib>;

#include "dash_x.hpp"

int main(int argc, char* argv[]) {
    set_project_paths();

    if (!generate_runtime_module()) {
        return 1;
    }

    copy_runtime_module();
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
