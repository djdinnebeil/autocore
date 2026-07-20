import base;

#include "dash_x.hpp"

/**
 * \brief Inserts the get_numkey_vk_code function into the runtime file.
 * \param runtime_file The output file stream to write the function to.
 */
void insert_vk_code(ofstream& runtime_file) {
    runtime_file << R"(export int get_numkey_vk_code(string_view vk_code_string) {
    static const unordered_map<string_view, int> numkey_vk_code_map = {
        {"numkey_0", numkey_0},
        {"numkey_1", numkey_1},
        {"numkey_2", numkey_2},
        {"numkey_3", numkey_3},
        {"numkey_4", numkey_4},
        {"numkey_5", numkey_5},
        {"numkey_6", numkey_6},
        {"numkey_7", numkey_7},
        {"numkey_8", numkey_8},
        {"numkey_9", numkey_9},
        {"numkey_star", numkey_star},
        {"numkey_plus", numkey_plus},
        {"numkey_dot", numkey_dot},
        {"numkey_enter", numkey_enter},
        {"numkey_dash", numkey_dash},
        {"numkey_slash", numkey_slash},
        {"play_pause_key", play_pause_key},
        {"calculator_key", calculator_key},
        {"mail_key", mail_key},
        {"home_page_key", home_page_key},
    };
    auto it = numkey_vk_code_map.find(vk_code_string);
    return (it != numkey_vk_code_map.end()) ? it->second : -1;
}
)";
}

/**
 * \brief Inserts the special case handling for the function get_function_by_name.
 * \param runtime_file The output file stream to write the special case handling to.
 */
void insert_special_case_in_function_by_name(ofstream& runtime_file) {
    runtime_file << R"(    // Check for special case "make_print_choice"
    if (function_name.rfind("make_print_choice", 0) == 0) {
        size_t opening_quotation = function_name.find('"');
        size_t closing_quotation = function_name.find("\",");
        string choice_name = string(function_name.substr(opening_quotation + 1, closing_quotation - opening_quotation - 1));
        bool bool_value = function_name.find("true", closing_quotation) != string::npos;
        return make_print_choice(choice_name, bool_value);
    }
)";
}

/**
 * \brief Processes a file to extract functions marked with \runtime and inserts them into the runtime file.
 * \param runtime_file The output file stream to write the functions to.
 * \param filepath The path to the file to process.
 */
void process_file(ofstream& runtime_file, const string& filepath) {
    cout << filepath << endl;
    ifstream file(filepath);

    if (!file.is_open()) {
        cerr << "Failed to open file: " << filepath << endl;
        return;
    }

    string line;

    while (getline(file, line)) {
        if (line.find(R"(* \runtime)") != string::npos) {
            cout << "Found \\runtime in: " << endl;
            string function_name;

            do {
                if (line.find(R"(*/)") != string::npos) {
                    cout << "Found */" << endl;
                    string function_line;
                    getline(file, function_line);

                    if (function_line.find("void ") != string::npos) {
                        size_t first_space = function_line.find(' ');
                        size_t parentheses = function_line.find("()");
                        function_name = function_line.substr(
                            first_space + 1,
                            parentheses - first_space - 1);
                        cout << function_name << endl;
                        break;
                    }
                }
            } while (getline(file, line));

            if (!function_name.empty()) {
                runtime_file
                    << "        {\""
                    << function_name
                    << "\", &"
                    << function_name
                    << "},\n";
            }
        }
    }

    file.close();
}

void process_directory(ofstream& runtime_file, const string& path) {
    try {
        if (!fs::is_directory(path)) {
            cout << "Provided path is not a directory: " << path << endl;
            return;
        }

        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                process_file(runtime_file, entry.path().string());
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        cerr << e.what() << endl;
    }
}

bool generate_runtime_module() {
    string module_description_comment = extract_module_description_comment();
    string vk_code_function_comment =
        extract_function_description_comment("get_numkey_vk_code");
    string function_by_name_function_comment =
        extract_function_description_comment("get_function_by_name");

    ofstream runtime_file(dash_x_path);

    if (!runtime_file.is_open()) {
        cerr << "Failed to create file: " << dash_x_path << endl;
        return false;
    }

    runtime_file
        << module_description_comment
        << "export module dash_x;\n"
        << "import core;\n\n"
        << vk_code_function_comment;

    insert_vk_code(runtime_file);

    runtime_file
        << "\n"
        << function_by_name_function_comment
        << "export function<void()> get_function_by_name(string_view function_name) {\n"
        << "    static const unordered_map<string_view, function<void()>> function_map = {\n";

    process_directory(runtime_file, main_import_path);
    process_directory(runtime_file, dll_import_path);
    process_directory(runtime_file, main_src_path);
    process_directory(runtime_file, dll_src_path);

    runtime_file << "    };\n";
    insert_special_case_in_function_by_name(runtime_file);
    runtime_file
        << "    // Lookup the function in the map\n"
        << "    auto it = function_map.find(function_name);\n"
        << "    return (it != function_map.end()) ? it->second : nullptr;\n"
        << "}\n";

    runtime_file.close();
    return true;
}

void copy_runtime_module() {
    fs::create_directory("keymap");
    string destination_path = R"(.\keymap\keymap_commands.ixx)";

    fs::copy_file(
        dash_x_path,
        destination_path,
        fs::copy_options::overwrite_existing);
}
