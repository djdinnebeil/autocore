module journal_commands;

import std;
import auto_core.core.console;
import auto_core.core.thread;
import command_registry;
import journal_clock;
import journal_component;
import journal_protocol;
import journal_title;
import auto_core.core.encoding;

import <Windows.h>;

namespace {

std::mutex title_action_mutex;
std::mutex prompt_mutex;

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
    const auto target_window =
        ac::console::focus_for_prompt_via_winkey();

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

void print_extended_timestamp() {
    journal_component().print_and_insert(
        journal_clock::get_extended_timestamp()
    );
}

} // namespace actions

void start_title_action(void (*action)()) {
    std::thread worker([action] {
        const std::scoped_lock lock {title_action_mutex};
        ac::thread::run_with_exception_handling(action, journal_component());
    });
    worker.detach();
}

std::optional<int> parse_choice_int(std::string_view value) {
    if (value.empty()) {
        return std::nullopt;
    }
    int parsed = 0;
    const auto result = std::from_chars(
        value.data(), value.data() + value.size(), parsed
    );
    if (result.ec != std::errc {} ||
        result.ptr != value.data() + value.size()) {
        return std::nullopt;
    }
    return parsed;
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
    int count = 1;

    if (!remainder.empty()) {
        if (remainder.front() != ',') {
            return {};
        }
        remainder = trim(remainder.substr(1));
        if (remainder.empty()) {
            return {};
        }

        const std::size_t comma = remainder.find(',');
        const std::string_view first = comma == std::string_view::npos
            ? remainder
            : trim(remainder.substr(0, comma));
        const std::string_view rest = comma == std::string_view::npos
            ? std::string_view {}
            : trim(remainder.substr(comma + 1));

        if (first == "true") {
            lower_bound = 0;
        }
        else if (first == "false") {
            lower_bound = 1;
        }
        else if (const auto parsed = parse_choice_int(first)) {
            lower_bound = *parsed;
        }
        else {
            return {};
        }

        if (!rest.empty()) {
            if (rest.find(',') != std::string_view::npos) {
                return {};
            }
            const auto parsed_count = parse_choice_int(rest);
            if (!parsed_count || *parsed_count < 1) {
                return {};
            }
            count = *parsed_count;
        }
    }

    return [name = std::move(name), lower_bound, count] {
        start_choice(name, lower_bound, count);
    };
}

command_registry::Action parse_print_and_insert(std::string_view arguments) {
    arguments = trim(arguments);
    if (arguments.empty() || arguments.front() != '"') {
        return {};
    }

    const std::size_t closing_quote = arguments.find('"', 1);
    if (closing_quote == std::string_view::npos) {
        return {};
    }

    std::string text {arguments.substr(1, closing_quote - 1)};
    if (!trim(arguments.substr(closing_quote + 1)).empty()) {
        return {};
    }

    return [text = std::move(text)] {
        journal_component().print_and_insert(text);
    };
}

} // namespace

command_registry::Registry create_journal_command_registry() {
    command_registry::Registry registry;
    registry.add(std::string {::print_extended_timestamp.name}, actions::print_extended_timestamp);
    registry.add(std::string {::print_episode_title.name}, [] {
        start_title_action(&journal_title::print_episode_title);
    });
    registry.add(std::string {::save_file_and_create_new_file.name}, [] {
        start_title_action(&journal_title::save_file_and_create_new_file);
    });
    registry.add_factory(
        "make_print_choice",
        parse_print_choice,
        R"(make_print_choice("", false))"
    );
    registry.add_factory(
        "print_and_insert_into_journal",
        parse_print_and_insert,
        R"(print_and_insert_into_journal(""))"
    );
    return registry;
}
