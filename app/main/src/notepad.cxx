module notepad;
import base;
import utils;
import thread;

import <algorithm>;
import <cctype>;

using std::transform;
using std::tolower;

string to_lower(const string& input) {
    string result = input;
    transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return tolower(c); });
    return result;
}

/**
* \brief Retrieves the openai api key.
*/
wstring set_openai_api_key() {
    ifstream openai_api_key_file(R"(.\notepad\openai_api_key.txt)");
    string line;
    getline(openai_api_key_file, line);
    openai_api_key_file.close();
    logg("openai_api_key set");
    wstring openai_api_key = str_to_wstr(line);
    return openai_api_key;
}

/**
 * \brief Prints the api key for OpenAI.
 *
 * This function prints the openai api key.
 *
 * \runtime
 */
void print_openai_api_key() {
    static wstring openai_api_key = set_openai_api_key();
    set_clipboard_text(openai_api_key);
    paste_from_clipboard();
    print("openai_api_key inserted");
}

wstring get_api_key() {
    logg("get_api_keys()");
    const string api_keys_path = R"(.\notepad\api_keys.txt)";
    ifstream file(api_keys_path);
    if (!file.is_open()) {
        print("error reading file");
        return L"";
    }
    vector<string> api_key_names;
    vector<string> api_key_values;

    string line;

    while (getline(file, line)) {
        size_t delimiter_pos = line.find('=');
        if (delimiter_pos != string::npos) {
            string name = line.substr(0, delimiter_pos);
            string value = line.substr(delimiter_pos + 1);
            api_key_names.push_back(name);
            api_key_values.push_back(value);
        }
    }

    file.close();
    oss api_key_prompt;
    api_key_prompt << "Enter the number:\n";
    for (size_t i = 0; i < api_key_names.size(); ++i) {
        string name = api_key_names[i];
        api_key_prompt << format("{}. for {}\n", i + 1, name);
    }
    api_key_prompt << "> ";
    printnl(api_key_prompt.str());

    string selection_str;
    int selection;
    while (true) {
        getline(cin, selection_str);
        logg("{}", selection_str);
        try {
            if (selection_str.empty() ) {
                selection_str = to_string(api_key_values.size());
            }
            selection = stoi(selection_str);
            if (selection == 0) {
                return L"##### \U00002705 Answer:";
            }
            else if (selection > api_key_values.size()) {
                printnl("Incorrect input\nEnter again: ");
            }
            else {
                print("{} inserted", to_lower(api_key_names[selection - 1]));
                return str_to_wstr(api_key_values[selection - 1]);
            }
        }
        catch (const invalid_argument&) {
            printnl("Incorrect input\nEnter again: ");
        }
    }
    logg("end of get_api_keys()");
}

/**
 * \brief Retrieves and prints a GPT message in a separate thread.
 *
 * This function runs the process of getting and printing a GPT message in a separate thread
 * to avoid blocking the main thread.
 */
void threaded_print_api_key() {
    wstring most_recent_clipboard_text = get_clipboard_text();
    HWND current_window_handle = GetForegroundWindow();
    set_focus_auto_core();
    wstring api_key = get_api_key();
    //wss ws;
    //ws << api_key.c_str();
    //set_clipboard_text(ws.str());
    set_clipboard_text(api_key);
    SetForegroundWindow(current_window_handle);
    paste_from_clipboard();
    Sleep(100);
    set_clipboard_text(most_recent_clipboard_text);
}

/**
 * \brief Prints the selected API key.
 *
 * Initiates the process of retrieving and printing an api key message in a separate thread.
 * \runtime
 */
void print_api_key() {
    logg("print_api_key()");
    thread t([=]() {run_with_exception_handling(threaded_print_api_key); });
    t.detach();
}
