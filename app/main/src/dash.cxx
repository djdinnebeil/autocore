module dash;

import std;

import config;
import dash_x;
import keymap;
import ac_component;

import <Windows.h>;

static std::string_view trim(std::string_view value) {
    const std::size_t first = value.find_first_not_of(" \t");

    if (first == std::string_view::npos) {
        return {};
    }

    const std::size_t last = value.find_last_not_of(" \t");

    return value.substr(first, last - first + 1);
}

static std::size_t find_action_delimiter(
    std::string_view value
) {
    int parenthesis_depth = 0;
    bool inside_quotes = false;

    for (std::size_t index = 0; index < value.size(); ++index) {
        const char character = value[index];

        if (character == '"' &&
            (index == 0 || value[index - 1] != '\\')) {
            inside_quotes = !inside_quotes;
            continue;
        }

        if (inside_quotes) {
            continue;
        }

        if (character == '(') {
            ++parenthesis_depth;
        }
        else if (character == ')') {
            if (parenthesis_depth == 0) {
                return std::string_view::npos;
            }

            --parenthesis_depth;
        }
        else if (character == ',' &&
            parenthesis_depth == 0) {
            return index;
        }
    }

    return std::string_view::npos;
}

/**
 * \brief Parses one keymap.ini entry.
 * \param line The complete keymap configuration line.
 * \param key_string Receives the configured key name.
 * \param primary Receives the primary command expression.
 * \param secondary Receives the secondary command expression.
 * \return true if the line contains a valid key and two actions;
 *         otherwise, false.
 */
static bool parse_line(
    std::string_view line,
    std::string_view& key_string,
    std::string_view& primary,
    std::string_view& secondary
) {
    const std::size_t opening_bracket = line.find('[');
    const std::size_t closing_bracket =
        line.find(']', opening_bracket);

    if (opening_bracket == std::string_view::npos ||
        closing_bracket == std::string_view::npos ||
        closing_bracket <= opening_bracket + 1) {
        return false;
    }

    const std::size_t opening_brace =
        line.find('{', closing_bracket);

    const std::size_t closing_brace =
        line.rfind('}');

    if (opening_brace == std::string_view::npos ||
        closing_brace == std::string_view::npos ||
        closing_brace <= opening_brace + 1) {
        return false;
    }

    const std::string_view actions = line.substr(
        opening_brace + 1,
        closing_brace - opening_brace - 1
    );

    const std::size_t delimiter =
        find_action_delimiter(actions);

    if (delimiter == std::string_view::npos) {
        return false;
    }

    key_string = trim(line.substr(
        opening_bracket + 1,
        closing_bracket - opening_bracket - 1
    ));

    primary = trim(actions.substr(0, delimiter));
    secondary = trim(actions.substr(delimiter + 1));

    return !key_string.empty() &&
        !primary.empty() &&
        !secondary.empty();
}

/**
 * \brief Parses the keymap configuration file and sets the keymap.
 *
 * This function reads keymap.ini, parses each configured key binding,
 * and populates the keymap with primary and secondary actions.
 */
void set_keymap_from_file() {
    std::ifstream config_file(R"(.\config\keymap.ini)");
    std::string line;

    if (ac::config::keymap_trace_enabled()) {
        auto_core.logg_and_logg("keymap trace logging enabled");
    }

    while (std::getline(config_file, line)) {
        std::string_view key_string;
        std::string_view primary;
        std::string_view secondary;

        if (!parse_line(line, key_string, primary, secondary)) {
            auto_core.logg_and_logg(
                "Invalid keymap line format: {}",
                line
            );
            continue;
        }

        const int key = get_numkey_vk_code(key_string);

        if (key == -1) {
            auto_core.logg_and_logg(
                "Unknown keymap key: {}",
                key_string
            );
            continue;
        }

        if (ac::config::keymap_trace_enabled()) {
            auto_core.logg_and_logg(
                "setting keymap entry: {} = {}, {}",
                key_string,
                primary,
                secondary
            );
        }

        ac_numkey_event[key] = {
            get_function_by_name(primary),
            get_function_by_name(secondary)
        };
    }

    auto_core.logg_and_logg("keymap configured from file");
}
