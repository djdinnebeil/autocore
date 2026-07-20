import base;
import <Windows.h>;

#include "dash_x.hpp"

string project_path;
string dash_x_path;
string main_import_path;
string dll_import_path;
string main_src_path;
string dll_src_path;
string build_project_cmd;
string auto_core_exe_path;
string runtime_map_hardlink_path;
string runtime_map_src_path;

void set_project_paths() {
    ifstream dash_x_file(R"(.\config\dash_x.ini)");
    string line;

    getline(dash_x_file, line);
    auto open_bracket = line.find('[');
    auto close_bracket = line.find(']');
    project_path = line.substr(open_bracket + 1, close_bracket - open_bracket - 1);

    getline(dash_x_file, line);
    open_bracket = line.find('[');
    close_bracket = line.find(']');
    build_project_cmd = line.substr(open_bracket + 1, close_bracket - open_bracket - 1);

    getline(dash_x_file, line);
    open_bracket = line.find('[');
    close_bracket = line.find(']');
    auto_core_exe_path = line.substr(open_bracket + 1, close_bracket - open_bracket - 1);

    dash_x_file.close();

    dash_x_path = project_path + R"(\app\main\modules\dash_x.ixx)";
    main_import_path = project_path + R"(\app\main\modules)";
    dll_import_path = project_path + R"(\app\core\modules)";
    main_src_path = project_path + R"(\app\main\src)";
    dll_src_path = project_path + R"(\app\core\src)";
    runtime_map_hardlink_path = auto_core_exe_path + R"(\keymap\keymap.ini)";
    runtime_map_src_path = auto_core_exe_path + R"(\config\keymap.ini)";
}

void create_hard_link() {
    fs::path hardlink_path = runtime_map_hardlink_path;
    fs::path source_path = runtime_map_src_path;

    std::error_code ec;

    if (fs::exists(hardlink_path, ec)) {
        return;
    }

    if (ec) {
        throw runtime_error(
            "Failed to check existing keymap.ini: " + ec.message());
    }

    if (!CreateHardLinkW(hardlink_path.c_str(), source_path.c_str(), nullptr)) {
        DWORD err = GetLastError();

        throw std::system_error(
            static_cast<int>(err),
            std::system_category(),
            "CreateHardLinkW failed");
    }
}
