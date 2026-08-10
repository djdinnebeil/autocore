import std;

#include "dash_x.hpp"

namespace fs = std::filesystem;

/**
 * \brief Inserts the get_numkey_vk_code function into the keymap command registry file.
 * \param keymap_file The output file stream to write the function to.
 */
void insert_vk_code(std::ofstream& keymap_file) {
    keymap_file << R"(export int get_numkey_vk_code(std::string_view vk_code_string) {
    static const std::unordered_map<std::string_view, int> numkey_vk_code_map = {
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
 * \param keymap_file The output file stream to write the special case handling to.
 */
void insert_special_case_in_function_by_name(std::ofstream& keymap_file) {
    keymap_file << R"(    // Check for special case "make_print_choice"
    if (function_name.rfind("make_print_choice", 0) == 0) {
        size_t opening_quotation = function_name.find('"');
        size_t closing_quotation = function_name.find("\",");
        std::string choice_name = std::string(function_name.substr(opening_quotation + 1, closing_quotation - opening_quotation - 1));
        bool bool_value = function_name.find("true", closing_quotation) != std::string::npos;
        return make_print_choice(choice_name, bool_value);
    }
)";
}

/**
 * \brief Processes a file to extract functions marked with \keymap_command and inserts them into the keymap file.
 * \param keymap_file The output file stream to write the functions to.
 * \param filepath The path to the file to process.
 */
void process_file(std::ofstream& keymap_file, const std::string& filepath) {
    std::cout << filepath << std::endl;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return;
    }

    std::string line;

    while (std::getline(file, line)) {
        if (line.find(R"(* \keymap_command)") != std::string::npos) {
            std::cout << "Found \\keymap_command in: " << std::endl;
            std::string function_name;

            do {
                if (line.find(R"(*/)") != std::string::npos) {
                    std::cout << "Found */" << std::endl;
                    std::string function_line;
                    std::getline(file, function_line);

                    if (function_line.find("void ") != std::string::npos) {
                        size_t first_space = function_line.find(' ');
                        size_t parentheses = function_line.find("()");
                        function_name = function_line.substr(
                            first_space + 1,
                            parentheses - first_space - 1);
                        std::cout << function_name << std::endl;
                        break;
                    }
                }
            } while (std::getline(file, line));

            if (!function_name.empty()) {
                keymap_file
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

void process_directory(std::ofstream& keymap_file, const std::string& path) {
    try {
        if (!fs::is_directory(path)) {
            std::cout << "Provided path is not a directory: " << path << std::endl;
            return;
        }

        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                process_file(keymap_file, entry.path().string());
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << e.what() << std::endl;
    }
}

bool generate_keymap_command_registry_module() {
    std::string module_description_comment = extract_module_description_comment();
    std::string vk_code_function_comment =
        extract_function_description_comment("get_numkey_vk_code");
    std::string function_by_name_function_comment =
        extract_function_description_comment("get_function_by_name");

    std::ofstream keymap_file(dash_x_path);

    if (!keymap_file.is_open()) {
        std::cerr << "Failed to create file: " << dash_x_path << std::endl;
        return false;
    }

    keymap_file
        << module_description_comment
        << "export module dash_x;\n\n"
        << "import std;\n"
        << "import ac_modules;\n\n"
        << vk_code_function_comment;

    insert_vk_code(keymap_file);

    keymap_file
        << "\n"
        << function_by_name_function_comment
        << "export std::function<void()> get_function_by_name(std::string_view function_name) {\n"
        << "    static const std::unordered_map<std::string_view, std::function<void()>> function_map = {\n";

    process_directory(keymap_file, main_import_path);
    process_directory(keymap_file, dll_import_path);
    process_directory(keymap_file, main_src_path);
    process_directory(keymap_file, dll_src_path);

    keymap_file << "    };\n";
    insert_special_case_in_function_by_name(keymap_file);
    keymap_file
        << "    // Lookup the function in the map\n"
        << "    auto it = function_map.find(function_name);\n"
        << "    return (it != function_map.end()) ? it->second : nullptr;\n"
        << "}\n";

    keymap_file.close();
    return true;
}

void copy_keymap_command_registry_module() {
    fs::create_directory("keymap");
    std::string destination_path = R"(.\keymap\keymap_commands.ixx)";

    fs::copy_file(
        dash_x_path,
        destination_path,
        fs::copy_options::overwrite_existing);
}
