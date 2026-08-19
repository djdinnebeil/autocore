/**
 * \file journal.ixx
 * \brief Follows the 'dice roll' method to generate a random number or prompt for journaling inspiration.
 *
 * This module provides functions to generate random choices for journaling prompts. It allows setting
 * the number of choices and generating random numbers within a specified range. It includes predefined
 * choices for various names, which can be used to inspire journaling entries.
 */
export module journal;

export import command_registry;
import std;

export namespace journal::runtime_commands {
    void register_with(command_registry::Registry& registry);
}

export {
    std::function<void()> make_print_choice(const std::string& name, bool include_zero);
    void print_Eric_choice();
    void print_Katrina_choice();
    void print_Lily_choice();
    void print_Star_choice();
    void print_Luna_choice();
    void print_Daniel_choice();
    void print_Jose_choice();
    void print_Jose_choices();
    void print_James_choice();
    void print_Jace_choice();
    void print_Tyler_choice();
    void print_Gin_choice();
    void print_Tabby_choice();
    void print_one_is_selected();
    void print_two_is_selected();
}
