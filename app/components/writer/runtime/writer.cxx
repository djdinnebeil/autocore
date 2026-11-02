module writer_commands;

import std;
import auto_core.core.console;
import auto_core.core.encoding;
import auto_core.core.paths;
import auto_core.core.thread;
import writer_component;

namespace {

const std::filesystem::path& session_prompts_file() {
    static const std::filesystem::path path =
        ac::paths::writer_directory() / "session_prompts.list";
    return path;
}

std::optional<std::vector<std::string>> load_session_prompts() {
    std::ifstream input(session_prompts_file());
    if (!input) {
        writer_component().log_alert(
            "Unable to open session prompts file: {}",
            session_prompts_file().string()
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
        writer_component().log_alert(
            "Unable to read session prompts file: {}",
            session_prompts_file().string()
        );
        return std::nullopt;
    }
    if (prompts.empty()) {
        writer_component().log_alert(
            "Session prompts file contains no prompts: {}",
            session_prompts_file().string()
        );
        return std::nullopt;
    }
    return prompts;
}

std::optional<std::size_t> select_numbered_entry(
    const std::string& menu,
    const std::size_t count,
    const char* const kind
) {
    writer_component().printnl(menu);

    std::string input;
    while (std::getline(std::cin, input)) {
        writer_component().log_info("{} selection: {}", kind, input);
        std::size_t selection = 0;
        const char* const first = input.data();
        const char* const last = first + input.size();
        const auto result = std::from_chars(first, last, selection);
        if (result.ec == std::errc {} && result.ptr == last &&
            selection > 0 && selection <= count) {
            return selection - 1;
        }
        writer_component().printnl("Incorrect input\nEnter again: ");
    }

    writer_component().log_alert(
        "{} selection cancelled: input closed",
        kind
    );
    return std::nullopt;
}

std::optional<std::string> select_session_prompt(
    const std::vector<std::string>& prompts
) {
    std::ostringstream menu;
    menu << "Select a session prompt:\n";
    for (std::size_t index = 0; index < prompts.size(); ++index) {
        menu << std::format("{}. {}\n", index + 1, prompts[index]);
    }
    menu << "> ";

    const auto selection = select_numbered_entry(
        menu.str(),
        prompts.size(),
        "Session prompt"
    );
    if (!selection) {
        return std::nullopt;
    }
    return prompts[*selection];
}

void select_and_insert_session_prompt_worker() {
    const auto target_window = ac::console::focus_for_prompt_via_winkey();
    if (!target_window) {
        writer_component().log_alert(
            ac::console::error_message(target_window.error())
        );
        return;
    }

    const auto prompts = load_session_prompts();
    if (!prompts) {
        return;
    }
    const auto prompt = select_session_prompt(*prompts);
    if (!prompt) {
        return;
    }

    writer_component().print(*prompt);
    writer_component().insert_text_preserving_clipboard_text(
        *target_window,
        ac::encoding::to_utf16(*prompt)
    );
}

const std::filesystem::path& profile_urls_file() {
    static const std::filesystem::path path =
        ac::paths::writer_directory() / "profile_urls.txt";
    return path;
}

std::string_view trim_spaces_and_tabs(const std::string_view text) {
    const auto first = text.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

struct ProfileUrl {
    std::string name;
    std::string url;
};

std::optional<std::vector<ProfileUrl>> load_profile_urls() {
    std::ifstream input(profile_urls_file());
    if (!input) {
        writer_component().log_alert(
            "Unable to open profile URLs file: {}",
            profile_urls_file().string()
        );
        return std::nullopt;
    }

    std::vector<ProfileUrl> profiles;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            writer_component().log_alert(
                "Skipping invalid profile URL line: {}",
                line
            );
            continue;
        }

        const std::string_view text {line};
        const auto name = trim_spaces_and_tabs(text.substr(0, separator));
        const auto url = trim_spaces_and_tabs(text.substr(separator + 1));
        if (name.empty() || url.empty()) {
            writer_component().log_alert(
                "Skipping invalid profile URL line: {}",
                line
            );
            continue;
        }
        profiles.push_back(ProfileUrl {
            std::string {name},
            std::string {url}
        });
    }

    if (input.bad()) {
        writer_component().log_alert(
            "Unable to read profile URLs file: {}",
            profile_urls_file().string()
        );
        return std::nullopt;
    }
    if (profiles.empty()) {
        writer_component().log_alert(
            "Profile URLs file contains no profile URLs: {}",
            profile_urls_file().string()
        );
        return std::nullopt;
    }
    return profiles;
}

std::optional<std::string> select_profile_url(
    const std::vector<ProfileUrl>& profiles
) {
    std::ostringstream menu;
    menu << "Select a profile URL:\n";
    for (std::size_t index = 0; index < profiles.size(); ++index) {
        menu << std::format("{}. {}\n", index + 1, profiles[index].name);
    }
    menu << "> ";

    const auto selection = select_numbered_entry(
        menu.str(),
        profiles.size(),
        "Profile URL"
    );
    if (!selection) {
        return std::nullopt;
    }
    return profiles[*selection].url;
}

void select_and_insert_profile_url_worker() {
    const auto target_window = ac::console::focus_for_prompt_via_winkey();
    if (!target_window) {
        writer_component().log_alert(
            ac::console::error_message(target_window.error())
        );
        return;
    }

    const auto profiles = load_profile_urls();
    if (!profiles) {
        return;
    }
    const auto url = select_profile_url(*profiles);
    if (!url) {
        return;
    }

    writer_component().print(*url);
    writer_component().insert_text_preserving_clipboard_text(
        *target_window,
        ac::encoding::to_utf16(*url),
        false
    );
}

} // namespace

void writer_actions::select_and_insert_session_prompt() {
    writer_component().log_info("select_and_insert_session_prompt()");

    auto lease = SelectionLease::try_acquire();
    if (!lease) {
        writer_component().log_alert(
            "A Writer selection menu is already active"
        );
        return;
    }

    std::thread worker([lease = std::move(*lease)]() mutable {
        ac::thread::run_with_exception_handling(
            select_and_insert_session_prompt_worker,
            writer_component()
        );
        (void)lease;
    });
    worker.detach();
}

void writer_actions::select_and_insert_profile_url() {
    writer_component().log_info("select_and_insert_profile_url()");

    auto lease = SelectionLease::try_acquire();
    if (!lease) {
        writer_component().log_alert(
            "A Writer selection menu is already active"
        );
        return;
    }

    std::thread worker([lease = std::move(*lease)]() mutable {
        ac::thread::run_with_exception_handling(
            select_and_insert_profile_url_worker,
            writer_component()
        );
        (void)lease;
    });
    worker.detach();
}
