/**
\file dash_x.cxx
\brief Generates the dash_x.ixx runtime configuration module for Auto Core.

The dash component reads its configured project paths, regenerates the runtime
function map, copies the generated module into the local keymap directory,
creates the keymap hard link when needed, and optionally rebuilds Auto Core.
*/
import base;
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
        cout << "Finished building dash_x" << endl;
    }
    else {
        cout << "\n\nBuild Auto Core? Press any key to continue" << endl;
        cin.get();

        int result = system(build_project_cmd.c_str());

        if (result != 0) {
            cerr << "Build failed with a return code of " << result << endl;
        }
    }

    cout << "Press any key to exit." << endl;
    cin.get();
    return 0;
}
