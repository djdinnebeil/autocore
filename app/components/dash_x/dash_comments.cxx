import base;

#include "dash_x.hpp"

/**
 * \brief Extracts the module description comment from the dash_x.ixx file.
 * \return The module description comment as a string.
 */
string extract_module_description_comment() {
    string description;
    ifstream file(dash_x_path);

    if (!file.is_open()) {
        cerr << "Failed to open file: " << dash_x_path << endl;
        return "";
    }

    string line;
    getline(file, line);

    if (line.starts_with("/*")) {
        description += line + "\n";

        while (getline(file, line)) {
            description += line + "\n";

            if (line.find("*/") != string::npos) {
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
string extract_function_description_comment(const string& function_name) {
    ifstream file(dash_x_path);

    if (!file.is_open()) {
        cerr << "Failed to open file: " << dash_x_path << endl;
        return "";
    }

    string line;
    string comment;
    getline(file, line);

    while (getline(file, line)) {
        comment = "";

        if (line.find("/*") != string::npos) {
            comment += line + "\n";

            while (getline(file, line)) {
                comment += line + "\n";

                if (line.find("*/") != string::npos) {
                    getline(file, line);
                    break;
                }
            }

            if (line.find(function_name) != string::npos) {
                file.close();
                return comment;
            }
        }
    }

    file.close();
    return "";
}
