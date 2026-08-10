#pragma once

extern std::string project_path;
extern std::string dash_x_path;
extern std::string main_import_path;
extern std::string dll_import_path;
extern std::string main_src_path;
extern std::string dll_src_path;
extern std::string build_project_cmd;
extern std::string auto_core_exe_path;
extern std::string keymap_hardlink_path;
extern std::string keymap_src_path;

void set_project_paths();
void create_hard_link();

std::string extract_module_description_comment();
std::string extract_function_description_comment(const std::string& function_name);

void insert_vk_code(std::ofstream& keymap_file);
void insert_special_case_in_function_by_name(std::ofstream& keymap_file);
void process_file(std::ofstream& keymap_file, const std::string& filepath);
void process_directory(std::ofstream& keymap_file, const std::string& path);
bool generate_keymap_command_registry_module();
void copy_keymap_command_registry_module();
