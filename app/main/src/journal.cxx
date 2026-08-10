module journal;

import std;
import keyboard;
import ac_component;
import print;
import thread;

import ac_main;
import <Windows.h>;

int upper_choice;
std::string choice_selector;

/**
 * \brief Generates a random number within a specified range.
 *
 * \param range_low The lower bound of the range.
 * \param range_high The upper bound of the range.
 * \return A random number within the specified range.
 */
int get_random_number(int range_low, int range_high) {
    static std::mt19937 engine {std::random_device{}()};
    std::uniform_int_distribution<int> dist(range_low, range_high);
    return dist(engine);
}

/**
 * \brief Sets the number of choices for the random selection.
 *
 * Prompts the user to enter the number of choices. If the input is invalid or empty,
 * it defaults to 2 choices.
 */
void set_number_of_choices() {
    HWND currentWindowHandle = GetForegroundWindow();
    set_focus_auto_core();
    std::string choice_selection;
    std::string choice_number_str;
    auto_core.printnl("Enter number of choices: ");
    std::getline(std::cin, choice_number_str);
    if (choice_number_str.empty()) {
        upper_choice = 2;
        auto_core.logg_and_logg("");
    }
    else {
        auto_core.logg_and_logg("{}", choice_number_str);
        try {
            int choice_number = stoi(choice_number_str);
            if (choice_number >= 2) {
                upper_choice = choice_number;
            }
            else if (choice_number == 1 || choice_number == 0) {
                upper_choice = 1;
            }
        }
        catch (...) {
            upper_choice = 2;
        }
    }
    SetForegroundWindow(currentWindowHandle);
}

/**
 * \brief Generates and prints a random choice with a name.
 *
 * Generates a random number within the range of 1 to upper_choice and prints it to the screen.
 */
void thread_print_choice_with_name() {
    set_number_of_choices();
    int random_choice_number = get_random_number(1, upper_choice);
    auto_core.print_and_insert(std::format("{} selects {}.", choice_selector, random_choice_number));
}

/**
 * \brief Generates and prints a random choice with zero included.
 *
 * Generates a random number within the range of 0 to upper_choice and prints it to the screen.
 */
void thread_print_choice_with_zero() {
    set_number_of_choices();
    int random_choice_number = get_random_number(0, upper_choice);
    auto_core.print_and_insert(std::format("{} selects {}.", choice_selector, random_choice_number));
}

/**
 * \brief Generates and prints a random choice with zero included.
 *
 * Generates a random number within the range of 0 to upper_choice and prints it to the screen.
 */
void thread_print_choice_with_custom(int lower_bound) {
    set_number_of_choices();
    if (upper_choice < lower_bound) {
        upper_choice = lower_bound + 1;
    }
    int random_choice_number = get_random_number(lower_bound, upper_choice);
    auto_core.print_and_insert(std::format("{} selects {}.", choice_selector, random_choice_number));
}

/**
 * \brief Generates and prints a random choice with zero included.
 *
 * Generates a random number within the range of 0 to upper_choice and prints it to the screen.
 */
void thread_print_choice_for_jose() {
    set_number_of_choices();
    int random_choice_number_1 = get_random_number(0, upper_choice);
    int random_choice_number_2 = get_random_number(0, upper_choice);
    int random_choice_number_3 = get_random_number(0, upper_choice);

    std::string random_choice_str_1 = std::format("{} selects {}.\n", choice_selector, random_choice_number_1);
    std::string random_choice_str_2 = std::format("{} selects {}.\n", choice_selector, random_choice_number_2);
    std::string random_choice_str_3 = std::format("{} selects {}.", choice_selector, random_choice_number_3);

    std::string choices_str = random_choice_str_1 + random_choice_str_2 + random_choice_str_3;
        
     
    auto_core.print_and_insert(std::format("{}", choices_str));
}


/**
 * \brief Prints a random choice for a given name.
 *
 * \param name The name associated with the choice.
 * \param include_zero Indicates if the range should include zero.
 */
void print_choice(const std::string& name, bool include_zero) {
    auto_core.logg_and_logg("print_choice()");
    choice_selector = name;
    if (include_zero) {
        std::thread t([=]() {ac::thread::run_with_exception_handling(thread_print_choice_with_zero, auto_core); });
        t.detach();
    }
    else {
        std::thread t([=]() {ac::thread::run_with_exception_handling(thread_print_choice_with_name, auto_core); });
        t.detach();
    }
}

/**
 * \brief Creates a function that prints a random choice for a given name.
 *
 * \param name The name associated with the choice.
 * \param include_zero Indicates if the range should include zero.
 * \return A function that prints the random choice.
 * 
 * \keymap_command
 */
std::function<void()> make_print_choice(const std::string& name, bool include_zero = false) {
    return [=]() {print_choice(name, include_zero); };
}

/** \keymap_command */
void print_Tabby_choice() {
    auto_core.logg_and_logg("print_Tabby_choice()");
    choice_selector = "Tabby";
    std::thread t([=]() {ac::thread::run_with_exception_handling(thread_print_choice_with_zero, auto_core); });
    t.detach();
}

/** \keymap_command */
void print_Eric_choice() {
    auto_core.logg_and_logg("print_Eric_choice()");
    choice_selector = "Eric";
    std::thread t([=]() {ac::thread::run_with_exception_handling(thread_print_choice_with_zero, auto_core); });
    t.detach();
}

/** \keymap_command */
void print_Katrina_choice() {
    auto_core.logg_and_logg("print_Katrina_choice()");
    choice_selector = "Katrina";
    std::thread t([=]() {ac::thread::run_with_exception_handling(thread_print_choice_with_zero, auto_core); });
    t.detach();
}

/** \keymap_command */
void print_Lily_choice() {
    auto_core.logg_and_logg("print_Lily_choice()");
    choice_selector = "Lily";
    std::thread t([=]() {ac::thread::run_with_exception_handling(thread_print_choice_with_name, auto_core); });
    t.detach();
}

/** \keymap_command */
void print_Star_choice() {
    auto_core.logg_and_logg("print_Star_choice()");
    choice_selector = "Star";
    std::thread t([=]() {ac::thread::run_with_exception_handling(thread_print_choice_with_zero, auto_core); });
    t.detach();
}

/** \keymap_command */
void print_Luna_choice() {
    auto_core.logg_and_logg("print_Luna_choice()");
    choice_selector = "Luna";
    std::thread t([=]() {ac::thread::run_with_exception_handling(thread_print_choice_with_name, auto_core); });
    t.detach();
}

/** \keymap_command */
void print_Daniel_choice() {
    auto_core.logg_and_logg("print_Daniel_choice()");
    choice_selector = "Daniel";
    std::thread t([=]() {ac::thread::run_with_exception_handling(thread_print_choice_with_name, auto_core); });
    t.detach();
}

/** \keymap_command */
void print_Jose_choice() {
    auto_core.logg_and_logg("print_Jose_choice()");
    choice_selector = "Jose";
    std::thread t([=]() {ac::thread::run_with_exception_handling(thread_print_choice_with_zero, auto_core); });
    t.detach();
}

/** \keymap_command */
void print_Jose_choices() {
    auto_core.logg_and_logg("print_Jose_choices()");
    choice_selector = "Jose";
    std::thread t([=]() {ac::thread::run_with_exception_handling(thread_print_choice_for_jose, auto_core); });
    t.detach();
}


/** \keymap_command */
void print_James_choice() {
    auto_core.logg_and_logg("print_James_choice()");
    choice_selector = "James";
    std::thread t([=]() {
        ac::thread::run_with_exception_handling([=]() {
            thread_print_choice_with_custom(3);
            }, auto_core);
        });
    t.detach();
}

/** \keymap_command */
void print_Jace_choice() {
    auto_core.logg_and_logg("print_Jace_choice()");
    choice_selector = "Jace";
    std::thread t([=]() {
        ac::thread::run_with_exception_handling([=]() {
            thread_print_choice_with_custom(3);
            }, auto_core);
        });
    t.detach();
}

/** \keymap_command */
void print_Tyler_choice() {
    auto_core.logg_and_logg("print_Tyler_choice()");
    choice_selector = "Tyler";
    std::thread t([=]() {
        ac::thread::run_with_exception_handling([=]() {
            thread_print_choice_with_custom(5);
            }, auto_core);
        });
    t.detach();
}

/** \keymap_command */
void print_Gin_choice() {
    auto_core.logg_and_logg("print_Gin_choice()");
    choice_selector = "Gin";
    std::thread t([=]() {
        ac::thread::run_with_exception_handling([=]() {
            thread_print_choice_with_custom(15);
            }, auto_core);
        });
    t.detach();
}

/** \keymap_command */
void print_one_is_selected() {
    auto_core.print_and_insert("1 is selected.");
}

/** \keymap_command */
void print_two_is_selected() {
    auto_core.print_and_insert("2 is selected.");
}
