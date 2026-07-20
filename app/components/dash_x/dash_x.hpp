#pragma once

extern string project_path;
extern string dash_x_path;
extern string main_import_path;
extern string dll_import_path;
extern string main_src_path;
extern string dll_src_path;
extern string build_project_cmd;
extern string auto_core_exe_path;
extern string runtime_map_hardlink_path;
extern string runtime_map_src_path;

void set_project_paths();
void create_hard_link();

string extract_module_description_comment();
string extract_function_description_comment(const string& function_name);

void insert_vk_code(ofstream& runtime_file);
void insert_special_case_in_function_by_name(ofstream& runtime_file);
void process_file(ofstream& runtime_file, const string& filepath);
void process_directory(ofstream& runtime_file, const string& path);
bool generate_runtime_module();
void copy_runtime_module();
