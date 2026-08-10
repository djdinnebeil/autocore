import std;
import config;
import encoding;
import <Windows.h>;

#include "dash_x.hpp"

namespace fs = std::filesystem;

std::string project_path;
std::string dash_x_path;
std::string main_import_path;
std::string dll_import_path;
std::string main_src_path;
std::string dll_src_path;
std::string build_project_cmd;
std::string auto_core_exe_path;
std::string keymap_hardlink_path;
std::string keymap_src_path;

void set_project_paths() {

    std::wstring executable_path_wstr = std::wstring {ac::config::executable_directory()};
    std::string config_file = ac::encoding::to_utf8(executable_path_wstr) + R"(\config\dash_x.ini)";
    std::ifstream dash_x_file(config_file);
    std::string line;

    std::getline(dash_x_file, line);
    auto open_bracket = line.find('[');
    auto close_bracket = line.find(']');
    project_path = line.substr(open_bracket + 1, close_bracket - open_bracket - 1);

    std::getline(dash_x_file, line);
    open_bracket = line.find('[');
    close_bracket = line.find(']');
    build_project_cmd = line.substr(open_bracket + 1, close_bracket - open_bracket - 1);

    std::getline(dash_x_file, line);
    open_bracket = line.find('[');
    close_bracket = line.find(']');
    auto_core_exe_path = line.substr(open_bracket + 1, close_bracket - open_bracket - 1);

    dash_x_file.close();

    std::cout << project_path << std::endl;
    std::cout << "project path is before this";

    dash_x_path = project_path + R"(\app\main\modules\dash_x.ixx)";
    main_import_path = project_path + R"(\app\main\modules)";
    dll_import_path = project_path + R"(\app\core\modules)";
    main_src_path = project_path + R"(\app\main\src)";
    dll_src_path = project_path + R"(\app\core\src)";
    keymap_hardlink_path = auto_core_exe_path + R"(\keymap\keymap.ini)";
    keymap_src_path = auto_core_exe_path + R"(\config\keymap.ini)";
}

void create_hard_link() {
    fs::path hardlink_path = keymap_hardlink_path;
    fs::path source_path = keymap_src_path;

    std::error_code ec;

    if (fs::exists(hardlink_path, ec)) {
        return;
    }

    if (ec) {
        throw std::runtime_error(
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
