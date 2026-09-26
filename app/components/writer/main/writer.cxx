module writer_commands;

import std;
import auto_core.core.console;
import auto_core.core.encoding;
import auto_core.core.paths;
import auto_core.core.thread;
import writer_component;

namespace {

const std::filesystem::path& gpt_prompts_file() {
    static const std::filesystem::path path =
        ac::paths::writer_directory() / "gpt_prompts.txt";
    return path;
}

std::optional<std::vector<std::string>> load_gpt_prompts() {
    std::ifstream input(gpt_prompts_file());
    if (!input) {
        writer_component().log_print(
            "Unable to open GPT prompts file: {}",
            gpt_prompts_file().string()
        );
        return std::nullopt;
    }

    std::vector<std::string> prompts;
    std::string prompt;
    while (std::getline(input, prompt)) {
        if (!prompt.empty() && prompt.back() == '\r') {
            prompt.pop_back();
        }
        if (prompt.empty()) {
            continue;
        }
        if (prompt.front() == '*') {
            prompt.erase(prompt.begin());
        }
        if (!prompt.empty()) {
            prompts.push_back(std::move(prompt));
        }
    }

    if (input.bad()) {
        writer_component().log_print(
            "Unable to read GPT prompts file: {}",
            gpt_prompts_file().string()
        );
        return std::nullopt;
    }
    if (prompts.empty()) {
        writer_component().log_print(
            "GPT prompts file contains no prompts: {}",
            gpt_prompts_file().string()
        );
        return std::nullopt;
    }
    return prompts;
}

std::optional<std::string> select_gpt_prompt(
    const std::vector<std::string>& prompts
) {
    std::ostringstream menu;
    menu << "Select a GPT prompt:\n";
    for (std::size_t index = 0; index < prompts.size(); ++index) {
        menu << std::format("{}. {}\n", index + 1, prompts[index]);
    }
    menu << "> ";
    writer_component().printnl(menu.str());

    std::string input;
    while (std::getline(std::cin, input)) {
        writer_component().log_main(
            "GPT prompt selection: {}", input
        );
        std::size_t selection = 0;
        const char* const first = input.data();
        const char* const last = first + input.size();
        const auto result = std::from_chars(first, last, selection);
        if (result.ec == std::errc {} && result.ptr == last &&
            selection > 0 && selection <= prompts.size()) {
            return prompts[selection - 1];
        }
        writer_component().printnl("Incorrect input\nEnter again: ");
    }

    writer_component().log_print(
        "GPT prompt selection cancelled: input closed"
    );
    return std::nullopt;
}

void select_and_insert_gpt_prompt_worker() {
    const auto target_window = ac::console::focus_for_prompt_via_winkey();
    if (!target_window) {
        writer_component().log_print(
            ac::console::error_message(target_window.error())
        );
        return;
    }

    const auto prompts = load_gpt_prompts();
    if (!prompts) {
        return;
    }
    const auto prompt = select_gpt_prompt(*prompts);
    if (!prompt) {
        return;
    }

    writer_component().print(*prompt);
    writer_component().insert_text_preserving_clipboard_text(
        *target_window,
        ac::encoding::to_utf16(*prompt)
    );
}

} // namespace

void writer_actions::select_and_insert_gpt_prompt() {
    writer_component().log_main("select_and_insert_gpt_prompt()");

    static std::atomic_bool selection_in_progress = false;
    bool expected = false;
    if (!selection_in_progress.compare_exchange_strong(expected, true)) {
        writer_component().log_print(
            "GPT prompt selection is already active"
        );
        return;
    }

    try {
        std::thread worker([] {
            struct SelectionGuard {
                std::atomic_bool& active;
                ~SelectionGuard() { active.store(false); }
            } guard {selection_in_progress};

            ac::thread::run_with_exception_handling(
                select_and_insert_gpt_prompt_worker,
                writer_component()
            );
        });
        worker.detach();
    }
    catch (...) {
        selection_in_progress.store(false);
        throw;
    }
}
