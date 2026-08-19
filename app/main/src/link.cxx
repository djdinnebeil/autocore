module link;

import std;

import auto_core.clipboard;
import auto_core.console;
import ac_main;
import auto_core.thread;
import auto_core.encoding;
import auto_core.paths;

import <Windows.h>;

bool dash_selected = false;

std::string read_file_to_string() {
    std::ifstream file(
        ac::paths::link_directory() / "my_study.txt"
    );

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file ");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * \brief Formats a dash-separated prompt string.
 *
 * Splits the input string by dashes and formats it for better readability.
 *
 * \param str The input string to format.
 * \return The formatted string.
 */
std::string format_dash_prompt(const std::string& str) {
    std::vector<std::string> names;
    std::stringstream s(str);
    std::string name;
    char dash = '-';
    while (std::getline(s, name, dash)) {
        names.push_back(name);
    }
    std::ostringstream formatted_names;
    for (std::size_t i = 0; i < names.size(); i++) {
        formatted_names << names[i];
        if ((i + 1) == 35) {
            formatted_names << ", ";
        }
        else if (((i + 1) % 5 == 0) && (i + 1 != names.size())) {
            formatted_names << "";
        }
        else if (i + 1 != names.size()) {
            formatted_names << ", ";
        }
    }
    auto_core.loggnl_and_loggnl("format_dash_prompt: ");
    auto formatted_names_str = formatted_names.str();
    return formatted_names_str;
}

/**
 * \brief Retrieves a GPT message from a file.
 *
 * Reads prompts from a file, formats them if necessary, and presents them for user selection.
 *
 * \return The selected GPT message.
 */
std::string get_gpt_message() {
    auto_core.logg_and_logg("get_gpt_message()");
    const std::filesystem::path gpt_prompts_path =
        ac::paths::link_directory() / "gpt_prompts.rc";
    std::ifstream file(gpt_prompts_path);
    if (!file.is_open()) {
        auto_core.print("error reading file");
        return "";
    }
    std::vector<std::string> gpt_prompts;
    std::string prompt;
    while (std::getline(file, prompt)) {
        if (prompt.find("*") != std::string::npos && prompt[0] == '*') {
            prompt = prompt.substr(1);
            gpt_prompts.push_back(prompt);
        }
        else if (prompt.find(" ") == std::string::npos) {
            prompt = format_dash_prompt(prompt);
            gpt_prompts.push_back(prompt);
        }
        else {
            gpt_prompts.push_back(prompt);
        }
    }
    file.close();
    std::ostringstream gpt_prompt_choice;
    gpt_prompt_choice << "Enter the number:\n";
    for (std::size_t i = 0; i < gpt_prompts.size(); ++i) {
        prompt = gpt_prompts[i];
        gpt_prompt_choice << format("{}. for {}\n", i + 1, prompt);
    }
    gpt_prompt_choice << "> ";
    auto_core.printnl(gpt_prompt_choice.str());
    std::string selection_str;
    int selection;
    while (true) {
        std::getline(std::cin, selection_str);
        auto_core.logg_and_logg("{}", selection_str);
        try {
            if (selection_str.empty()) {
                std::string file_output = read_file_to_string();
                if (file_output.empty()) {
                    return gpt_prompts[gpt_prompts.size() - 1];
                }
                return file_output;
            }
            selection = stoi(selection_str);
            if (selection == 0 || selection > gpt_prompts.size()) {
                auto_core.printnl("Incorrect input\nEnter again: ");
            }
            else if (selection == 5) {
                dash_selected = true;
                return gpt_prompts[selection - 1];
            }
            else {
                return gpt_prompts[selection - 1];
            }
        }
        catch (const std::invalid_argument&) {
            auto_core.printnl("Incorrect input\nEnter again: ");
        }
    }
    auto_core.logg_and_logg("end of get_gpt_message()");
}

/**
 * \brief Retrieves and prints a GPT message in a separate thread.
 *
 * This function runs the process of getting and printing a GPT message in a separate thread
 * to avoid blocking the main thread.
 */
void threaded_print_gpt_message() {
    dash_selected = false;

    const auto target_window = ac::console::focus_for_prompt();

    if (!target_window) {
        auto_core.logg_and_print(
            ac::console::error_message(target_window.error())
        );
        return;
    }

    std::string gpt_message = get_gpt_message();

    auto_core.print(gpt_message);

    std::wstring clipboard_text =
        ac::encoding::to_utf16(gpt_message);

    auto_core.insert_text_preserving_clipboard_text(
        *target_window,
        clipboard_text
    );
}

/**
 * \brief Prints a GPT message.
 *
 * Initiates the process of retrieving and printing a GPT message in a separate thread.
 * \keymap_command
 */
void print_gpt_message() {
    auto_core.logg_and_logg("print_gpt_message()");
    std::thread t([=]() {ac::thread::run_with_exception_handling(threaded_print_gpt_message, auto_core); });
    t.detach();
}

void link::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("print_gpt_message", &::print_gpt_message);
}
