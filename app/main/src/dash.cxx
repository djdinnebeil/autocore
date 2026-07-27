module dash;
import ac_modules;
import keymap;
import dash_x;
import config;
import <Windows.h>;

// Internal helper to parse a keymap.ini line
static bool parse_line(std::string_view line, std::string_view& key_string, std::string_view& primary, std::string_view& secondary) {
    size_t opening_bracket = line.find('[');
    size_t closing_bracket = line.find(']');
    if (opening_bracket == std::string_view::npos || closing_bracket == std::string_view::npos || closing_bracket <= opening_bracket) {
        return false;
    }
    key_string = line.substr(opening_bracket + 1, closing_bracket - opening_bracket - 1);
    size_t opening_brace = line.find('{');
    size_t closing_brace = line.find('}');
    if (opening_brace == std::string_view::npos || closing_brace == std::string_view::npos || closing_brace <= opening_brace) {
        return false;
    }
    size_t comma_after_parenthese = line.find("),");
    size_t comma_delimiter = line.find(',');
    if (comma_after_parenthese != std::string_view::npos) {
        comma_delimiter = comma_after_parenthese + 1;
    }
    if (comma_delimiter == std::string_view::npos || comma_delimiter <= opening_brace || comma_delimiter >= closing_brace) {
        return false;
    }
    primary = line.substr(opening_brace + 1, comma_delimiter - opening_brace - 1);
    secondary = line.substr(comma_delimiter + 2, closing_brace - comma_delimiter - 2);
    return true;
}

/**
 * \brief Parses the runtime configuration file and sets the keymap.
 *
 * This function reads keymap.ini, parses each configured key binding,
 * and populates the keymap with primary and secondary actions.
 */
void set_keymap_from_file() {
    std::ifstream config_file(R"(.\config\keymap.ini)");
    std::string line;
    std::ostringstream log_buffer;
    bool log_buffer_empty = true;
    while (std::getline(config_file, line)) {
        std::string_view key_string, primary, secondary;
        if (!parse_line(line, key_string, primary, secondary)) {
            ac::logger::logg("Invalid line format: {}", line);
            continue;
        }
        int key = get_numkey_vk_code(key_string);
        if (key != -1) {
            ac_numkey_event[key] = {get_function_by_name(primary), get_function_by_name(secondary)};
        }
        // Logging, if enabled
        if (ac::config.runtime_debugger) {
            ac::logger::logg("{} = {}, {}", key, primary, secondary);
        }
        else if (ac::config.runtime_logger) {
            if (!log_buffer_empty) {
                log_buffer << "\n";
            }
            log_buffer << key << " = " << primary << ", " << secondary;
            log_buffer_empty = false;
        }
    }
    if (ac::config.runtime_debugger) {
        ac::print("runtime logger set to debug mode");
    }
    else if (ac::config.runtime_logger) {
        ac::logger::logg(log_buffer.str());
    }
    ac::logger::logg("runtime map configured");
    config_file.close();
}
