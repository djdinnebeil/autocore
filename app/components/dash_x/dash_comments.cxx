import std;

#include "dash_x.hpp"

/**
 * \brief Extracts the module description comment from the dash_x.ixx file.
 * \return The module description comment as a string.
 */
std::string extract_module_description_comment() {
    std::string description;
    std::ifstream file(dash_x_path);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << dash_x_path << std::endl;
        return "";
    }

    std::string line;
    std::getline(file, line);

    if (line.starts_with("/*")) {
        description += line + "\n";

        while (std::getline(file, line)) {
            description += line + "\n";

            if (line.find("*/") != std::string::npos) {
                break;
            }
        }
    }

    file.close();
    return description;
}

/**
 * \brief Extracts the function description comment for a specific function from the dash_x.ixx file.
 * \param function_name The name of the function to extract the comment for.
 * \return The function description comment as a string.
 */
std::string extract_function_description_comment(const std::string& function_name) {
    std::ifstream file(dash_x_path);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << dash_x_path << std::endl;
        return "";
    }

    std::string line;
    std::string comment;
    std::getline(file, line);

    while (std::getline(file, line)) {
        comment = "";

        if (line.find("/*") != std::string::npos) {
            comment += line + "\n";

            while (getline(file, line)) {
                comment += line + "\n";

                if (line.find("*/") != std::string::npos) {
                    getline(file, line);
                    break;
                }
            }

            if (line.find(function_name) != std::string::npos) {
                file.close();
                return comment;
            }
        }
    }

    file.close();
    return "";
}
