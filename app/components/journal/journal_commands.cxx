module journal_commands;

import std;
import auto_core.console;
import auto_core.keyboard;
import auto_core.thread;
import command_registry;
import journal_clock;
import journal_component;
import journal_protocol;
import star;
import auto_core.encoding;

import <Windows.h>;
import <conio.h>;

namespace {

std::mutex star_action_mutex;
std::mutex prompt_mutex;
int auto_core_taskbar_position = -1;

std::string_view trim(std::string_view value) {
    const std::size_t first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

int random_number(int low, int high) {
    static thread_local std::mt19937 engine {std::random_device {}()};
    return std::uniform_int_distribution<int> {low, high}(engine);
}

std::optional<int> prompt_for_upper_choice() {
    const HWND previous_window = GetForegroundWindow();
    std::expected<ac::console::WindowHandle, ac::console::Error> target_window;

    if (auto_core_taskbar_position >= 0) {
        ac::keyboard::send_winkey(auto_core_taskbar_position);
        if (const auto activated = ac::console::activate(); !activated) {
            target_window = std::unexpected(activated.error());
        }
        else {
            while (_kbhit()) {
                (void)_getch();
            }
            std::cin.clear();
            target_window = previous_window;
        }
    }
    else {
        target_window = ac::console::focus_for_prompt();
    }

    if (!target_window) {
        journal_component().logg_and_print(
            ac::console::error_message(target_window.error())
        );
        return std::nullopt;
    }

    journal_component().printnl("Enter number of choices: ");
    std::string value;
    std::getline(std::cin, value);

    int upper = 2;
    if (!value.empty()) {
        const auto parsed = std::from_chars(
            value.data(), value.data() + value.size(), upper
        );
        if (parsed.ec != std::errc {} || parsed.ptr != value.data() + value.size()) {
            upper = 2;
        }
    }

    upper = (std::max)(upper, 1);

    journal_component().logg_and_logg("{}", upper);

    SetForegroundWindow(static_cast<HWND>(*target_window));
    return upper;
}

void run_choice(std::string name, int lower_bound, int count = 1) {
    const std::scoped_lock prompt_lock {prompt_mutex};
    const auto upper_choice = prompt_for_upper_choice();
    if (!upper_choice) {
        return;
    }

    const int upper = *upper_choice < lower_bound
        ? lower_bound + 1
        : *upper_choice;
    std::string output;
    for (int index = 0; index < count; ++index) {
        if (!output.empty()) {
            output += '\n';
        }
        output += std::format(
            "{} selects {}.",
            name,
            random_number(lower_bound, upper)
        );
    }
    journal_component().print_and_insert(output);
}

void start_choice(std::string name, int lower_bound, int count = 1) {
    std::thread worker([
        name = std::move(name), lower_bound, count
    ] {
        ac::thread::run_with_exception_handling(
            [name, lower_bound, count] {
                run_choice(name, lower_bound, count);
            },
            journal_component()
        );
    });
    worker.detach();
}

namespace actions {

void print_Tabby_choice() { start_choice("Tabby", 0); }
void print_Eric_choice() { start_choice("Eric", 0); }
void print_Katrina_choice() { start_choice("Katrina", 0); }
void print_Lily_choice() { start_choice("Lily", 1); }
void print_Star_choice() { start_choice("Star", 0); }
void print_Luna_choice() { start_choice("Luna", 1); }
void print_Daniel_choice() { start_choice("Daniel", 1); }
void print_Jose_choice() { start_choice("Jose", 0); }
void print_Jose_choices() { start_choice("Jose", 0, 3); }
void print_James_choice() { start_choice("James", 3); }
void print_Jace_choice() { start_choice("Jace", 3); }
void print_Tyler_choice() { start_choice("Tyler", 5); }
void print_Gin_choice() { start_choice("Gin", 15); }
void print_Gianna_choice() { start_choice("Gianna", 1); }

void print_one_is_selected() {
    journal_component().print_and_insert("1 is selected.");
}

void print_two_is_selected() {
    journal_component().print_and_insert("2 is selected.");
}

void print_extended_timestamp() {
    journal_component().print_and_insert(
        journal_clock::get_extended_timestamp()
    );
}

} // namespace actions

void start_star_action(void (*action)()) {
    std::thread worker([action] {
        const std::scoped_lock lock {star_action_mutex};
        ac::thread::run_with_exception_handling(action, journal_component());
    });
    worker.detach();
}

command_registry::Action parse_print_choice(std::string_view arguments) {
    arguments = trim(arguments);
    if (arguments.empty() || arguments.front() != '"') {
        return {};
    }

    const std::size_t closing_quote = arguments.find('"', 1);
    if (closing_quote == std::string_view::npos) {
        return {};
    }

    std::string name {arguments.substr(1, closing_quote - 1)};
    std::string_view remainder = trim(arguments.substr(closing_quote + 1));
    int lower_bound = 1;

    if (!remainder.empty()) {
        if (remainder.front() != ',') {
            return {};
        }
        remainder = trim(remainder.substr(1));
        if (remainder == "true") {
            lower_bound = 0;
        }
        else if (remainder != "false") {
            return {};
        }
    }

    return [name = std::move(name), lower_bound] {
        start_choice(name, lower_bound);
    };
}

} // namespace

void set_journal_taskbar_position(const int position) {
    auto_core_taskbar_position = position;
}

command_registry::Registry create_journal_command_registry() {
    command_registry::Registry registry;
    registry.add(std::string {::print_Tabby_choice.name}, actions::print_Tabby_choice);
    registry.add(std::string {::print_Eric_choice.name}, actions::print_Eric_choice);
    registry.add(std::string {::print_Katrina_choice.name}, actions::print_Katrina_choice);
    registry.add(std::string {::print_Lily_choice.name}, actions::print_Lily_choice);
    registry.add(std::string {::print_Star_choice.name}, actions::print_Star_choice);
    registry.add(std::string {::print_Luna_choice.name}, actions::print_Luna_choice);
    registry.add(std::string {::print_Daniel_choice.name}, actions::print_Daniel_choice);
    registry.add(std::string {::print_Jose_choice.name}, actions::print_Jose_choice);
    registry.add(std::string {::print_Jose_choices.name}, actions::print_Jose_choices);
    registry.add(std::string {::print_James_choice.name}, actions::print_James_choice);
    registry.add(std::string {::print_Jace_choice.name}, actions::print_Jace_choice);
    registry.add(std::string {::print_Tyler_choice.name}, actions::print_Tyler_choice);
    registry.add(std::string {::print_Gin_choice.name}, actions::print_Gin_choice);
    registry.add(std::string {::print_Gianna_choice.name}, actions::print_Gianna_choice);
    registry.add(std::string {::print_one_is_selected.name}, actions::print_one_is_selected);
    registry.add(std::string {::print_two_is_selected.name}, actions::print_two_is_selected);
    registry.add(std::string {::print_extended_timestamp.name}, actions::print_extended_timestamp);
    registry.add(std::string {::print_episode_title.name}, [] {
        start_star_action(&star_actions::print_episode_title);
    });
    registry.add(std::string {::save_file_and_create_new_file.name}, [] {
        start_star_action(&star_actions::save_file_and_create_new_file);
    });
    registry.add_factory(
        "make_print_choice",
        parse_print_choice,
        R"(make_print_choice("", false))"
    );
    return registry;
}
