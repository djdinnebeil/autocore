module journal;
import visual;
import keyboard;
import thread;
import ac_core;
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
    ac::printnl("Enter number of choices: ");
    std::getline(std::cin, choice_number_str);
    if (choice_number_str.empty()) {
        upper_choice = 2;
        ac::logger::logg("");
    }
    else {
        ac::logger::logg("{}", choice_number_str);
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
    ac::print_to_screen(std::format("{} selects {}.", choice_selector, random_choice_number));
}

/**
 * \brief Generates and prints a random choice with zero included.
 *
 * Generates a random number within the range of 0 to upper_choice and prints it to the screen.
 */
void thread_print_choice_with_zero() {
    set_number_of_choices();
    int random_choice_number = get_random_number(0, upper_choice);
    ac::print_to_screen(std::format("{} selects {}.", choice_selector, random_choice_number));
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
    ac::print_to_screen(std::format("{} selects {}.", choice_selector, random_choice_number));
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
        
     
    ac::print_to_screen(std::format("{}", choices_str));
}


/**
 * \brief Prints a random choice for a given name.
 *
 * \param name The name associated with the choice.
 * \param include_zero Indicates if the range should include zero.
 */
void print_choice(const std::string& name, bool include_zero) {
    ac::logger::logg("print_choice()");
    choice_selector = name;
    if (include_zero) {
        std::thread t([=]() {run_with_exception_handling(thread_print_choice_with_zero); });
        t.detach();
    }
    else {
        std::thread t([=]() {run_with_exception_handling(thread_print_choice_with_name); });
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
 * \runtime
 */
std::function<void()> make_print_choice(const std::string& name, bool include_zero = false) {
    return [=]() {print_choice(name, include_zero); };
}

/** \runtime */
void print_Tabby_choice() {
    ac::logger::logg("print_Tabby_choice()");
    choice_selector = "Tabby";
    std::thread t([=]() {run_with_exception_handling(thread_print_choice_with_zero); });
    t.detach();
}

/** \runtime */
void print_Eric_choice() {
    ac::logger::logg("print_Eric_choice()");
    choice_selector = "Eric";
    std::thread t([=]() {run_with_exception_handling(thread_print_choice_with_zero); });
    t.detach();
}

/** \runtime */
void print_Katrina_choice() {
    ac::logger::logg("print_Katrina_choice()");
    choice_selector = "Katrina";
    std::thread t([=]() {run_with_exception_handling(thread_print_choice_with_zero); });
    t.detach();
}

/** \runtime */
void print_Lily_choice() {
    ac::logger::logg("print_Lily_choice()");
    choice_selector = "Lily";
    std::thread t([=]() {run_with_exception_handling(thread_print_choice_with_name); });
    t.detach();
}

/** \runtime */
void print_Star_choice() {
    ac::logger::logg("print_Star_choice()");
    choice_selector = "Star";
    std::thread t([=]() {run_with_exception_handling(thread_print_choice_with_zero); });
    t.detach();
}

/** \runtime */
void print_Luna_choice() {
    ac::logger::logg("print_Luna_choice()");
    choice_selector = "Luna";
    std::thread t([=]() {run_with_exception_handling(thread_print_choice_with_name); });
    t.detach();
}

/** \runtime */
void print_Daniel_choice() {
    ac::logger::logg("print_Daniel_choice()");
    choice_selector = "Daniel";
    std::thread t([=]() {run_with_exception_handling(thread_print_choice_with_name); });
    t.detach();
}

/** \runtime */
void print_Jose_choice() {
    ac::logger::logg("print_Jose_choice()");
    choice_selector = "Jose";
    std::thread t([=]() {run_with_exception_handling(thread_print_choice_with_zero); });
    t.detach();
}

/** \runtime */
void print_Jose_choices() {
    ac::logger::logg("print_Jose_choices()");
    choice_selector = "Jose";
    std::thread t([=]() {run_with_exception_handling(thread_print_choice_for_jose); });
    t.detach();
}


/** \runtime */
void print_James_choice() {
    ac::logger::logg("print_James_choice()");
    choice_selector = "James";
    std::thread t([=]() {
        run_with_exception_handling([=]() {
            thread_print_choice_with_custom(3);
            });
        });
    t.detach();
}

/** \runtime */
void print_Jace_choice() {
    ac::logger::logg("print_Jace_choice()");
    choice_selector = "Jace";
    std::thread t([=]() {
        run_with_exception_handling([=]() {
            thread_print_choice_with_custom(3);
            });
        });
    t.detach();
}

/** \runtime */
void print_Tyler_choice() {
    ac::logger::logg("print_Tyler_choice()");
    choice_selector = "Tyler";
    std::thread t([=]() {
        run_with_exception_handling([=]() {
            thread_print_choice_with_custom(5);
            });
        });
    t.detach();
}

/** \runtime */
void print_Gin_choice() {
    ac::logger::logg("print_Gin_choice()");
    choice_selector = "Gin";
    std::thread t([=]() {
        run_with_exception_handling([=]() {
            thread_print_choice_with_custom(15);
            });
        });
    t.detach();
}

/** \runtime */
void print_one_is_selected() {
    ac::print_to_screen("1 is selected.");
}

/** \runtime */
void print_two_is_selected() {
    ac::print_to_screen("2 is selected.");
}
