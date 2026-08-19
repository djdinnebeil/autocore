module notepad;

import std;

import auto_core.encoding;
import auto_core.console;
import ac_main;
import auto_core.clipboard;
import auto_core.thread;
import auto_core.paths;

import <algorithm>;
import <cctype>;

using std::transform;
using std::tolower;

std::string to_lower(const std::string& input) {
    std::string result = input;
    transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return tolower(c); });
    return result;
}

/**
* \brief Retrieves the openai api key.
*/
std::wstring set_openai_api_key() {
    const std::filesystem::path openai_api_path =
        ac::paths::notepad_directory() / "openai_api_key.txt";

    std::ifstream openai_api_key_file(openai_api_path);
    std::string line;
    std::getline(openai_api_key_file, line);
    openai_api_key_file.close();
    auto_core.logg_and_logg("openai_api_key set");
    std::wstring openai_api_key = ac::encoding::to_utf16(line);
    return openai_api_key;
}

/**
 * \brief Prints the api key for OpenAI.
 *
 * This function prints the openai api key.
 *
 * \keymap_command
 */
void print_openai_api_key() {
    static std::wstring openai_api_key = set_openai_api_key();
    auto_core.insert_text_replacing_clipboard(openai_api_key);
    auto_core.print("openai_api_key inserted");
}

std::wstring get_api_key() {
    auto_core.logg_and_logg("get_api_keys()");

    const std::filesystem::path api_keys_path =
        ac::paths::notepad_directory() / "api_key.txt";

    std::ifstream file(api_keys_path);
    if (!file.is_open()) {
        auto_core.print("error reading file");
        return L"";
    }
    std::vector<std::string> api_key_names;
    std::vector<std::string> api_key_values;

    std::string line;

    while (std::getline(file, line)) {
        std::size_t delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            std::string name = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);
            api_key_names.push_back(name);
            api_key_values.push_back(value);
        }
    }

    file.close();
    std::ostringstream api_key_prompt;
    api_key_prompt << "Enter the number:\n";
    for (std::size_t i = 0; i < api_key_names.size(); ++i) {
        std::string name = api_key_names[i];
        api_key_prompt << format("{}. for {}\n", i + 1, name);
    }
    api_key_prompt << "> ";
    auto_core.printnl(api_key_prompt.str());

    std::string selection_str;
    int selection;
    while (true) {
        std::getline(std::cin, selection_str);
        auto_core.logg_and_logg("{}", selection_str);
        try {
            if (selection_str.empty() ) {
                selection_str = std::to_string(api_key_values.size());
            }
            selection = stoi(selection_str);
            if (selection == 0) {
                return L"##### \U00002705 Answer:";
            }
            else if (selection > api_key_values.size()) {
                auto_core.printnl("Incorrect input\nEnter again: ");
            }
            else {
                auto_core.print(
                    "{} inserted",
                    to_lower(api_key_names[selection - 1])
                );
                return ac::encoding::to_utf16(api_key_values[selection - 1]);
            }
        }
        catch (const std::invalid_argument&) {
            auto_core.printnl("Incorrect input\nEnter again: ");
        }
    }
    auto_core.logg_and_logg("end of get_api_keys()");
}

/**
 * \brief Retrieves and prints a GPT message in a separate thread.
 *
 * This function runs the process of getting and printing a GPT message in a separate thread
 * to avoid blocking the main thread.
 */
void threaded_print_api_key() {
    const auto target_window = ac::console::focus_for_prompt();

    if (!target_window) {
        auto_core.logg_and_print(
            ac::console::error_message(target_window.error())
        );
        return;
    }

    std::wstring api_key = get_api_key();

    auto_core.insert_text_preserving_clipboard_text(*target_window, api_key);
}

/**
 * \brief Prints the selected API key.
 *
 * Initiates the process of retrieving and printing an api key message in a separate thread.
 * \keymap_command
 */
void print_api_key() {
    auto_core.logg_and_logg("print_api_key()");
    std::thread t([=]() {ac::thread::run_with_exception_handling(threaded_print_api_key, auto_core); });
    t.detach();
}

void notepad::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("print_openai_api_key", &::print_openai_api_key);
    registry.add("print_api_key", &::print_api_key);
}
