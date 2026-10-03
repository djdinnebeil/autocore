#include "../shared/writer_data_detail.hpp"

import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;
import writer_defaults;
import writer_notepad;
import components_editor_request;

import <iostream>;
import auto_core.core.shell;

namespace {

ac::Component writer_editor {
    "writer_editor",
    ac::logging::config::LoggingScope {"writer"}
};

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool writer_initialized() {
    const auto path = ac::paths::config_directory() / "writer.ini";
    std::error_code error;
    return std::filesystem::exists(path, error) && !error;
}

bool require_initialization() {
    if (writer_initialized()) {
        return true;
    }
    writer_editor.log_print(
        "Writer must be initialized first. Run writer_config.exe --init."
    );
    return false;
}

bool append_line(const std::filesystem::path& file, std::string_view line) {
    std::error_code error;
    std::filesystem::create_directories(file.parent_path(), error);
    if (error) {
        writer_editor.log_print(
            "Failed to create {}: {}",
            file.parent_path().string(),
            error.message()
        );
        return false;
    }

    std::ofstream output(file, std::ios::binary | std::ios::app);
    if (!output) {
        writer_editor.log_print("Failed to append {}", file.string());
        return false;
    }
    output.write(line.data(), static_cast<std::streamsize>(line.size()));
    output.put('\n');
    output.close();
    if (!output) {
        writer_editor.log_print("Failed to append {}", file.string());
        return false;
    }
    return true;
}

enum class StoreState {
    missing,
    present,
    unreadable
};

StoreState inspect_store(const std::filesystem::path& path) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        writer_editor.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return StoreState::unreadable;
    }
    return present ? StoreState::present : StoreState::missing;
}

bool write_missing_file(
    const std::filesystem::path& file,
    const std::string_view contents
) {
    const auto state = inspect_store(file);
    if (state == StoreState::unreadable) {
        return false;
    }
    if (state == StoreState::present) {
        writer_editor.log_print("Left existing {} unchanged.", file.string());
        return true;
    }

    std::error_code error;
    std::filesystem::create_directories(file.parent_path(), error);
    if (error) {
        writer_editor.log_print(
            "Failed to create {}: {}",
            file.parent_path().string(),
            error.message()
        );
        return false;
    }

    std::ofstream output(file, std::ios::binary | std::ios::trunc);
    if (!output) {
        writer_editor.log_print("Failed to create {}", file.string());
        return false;
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    if (!output) {
        writer_editor.log_print("Failed to write {}", file.string());
        return false;
    }
    writer_editor.log_print("Created {}", file.string());
    return true;
}

bool create_notes_directory(const std::filesystem::path& notes) {
    std::error_code error;
    std::filesystem::create_directories(notes, error);
    if (error) {
        writer_editor.log_print(
            "Failed to create notes directory: {}",
            error.message()
        );
        return false;
    }
    writer_editor.log_print("Created {}", notes.string());
    return true;
}

int add_line(const std::filesystem::path& file, std::string_view prompt) {
    std::cout << prompt;
    std::string input;
    if (!std::getline(std::cin, input)) {
        return 1;
    }
    const auto line = trim(input);
    if (line.empty()) {
        return 0;
    }
    return append_line(file, line) ? 0 : 1;
}

int add_task() {
    return add_line(
        ac::paths::writer_directory() / "task_list.txt",
        "Task: "
    );
}

int add_session_prompt() {
    return add_line(
        ac::paths::writer_directory() / "session_prompts.list",
        "Session prompt: "
    );
}

int open_daily_note() {
    return writer_notepad::create_or_open_daily_note(writer_editor) ? 0 : 1;
}

bool cancelled(const std::string_view value) {
    if (value != "cancel") {
        return false;
    }
    writer_editor.log_print("Cancelled.");
    return true;
}

std::optional<std::string> prompt_recommended(std::string_view recommended) {
    std::cout << "Session prompt [" << recommended << "]: ";
    std::string input;
    if (!std::getline(std::cin, input)) {
        return std::nullopt;
    }
    const auto value = trim(input);
    if (cancelled(value)) {
        return std::nullopt;
    }
    if (value.empty()) {
        return std::string {recommended};
    }
    return std::string {value};
}

std::optional<std::string> prompt_task() {
    std::cout << "Task [" << writer::defaults::task_line << "]: ";
    std::string input;
    if (!std::getline(std::cin, input)) {
        return std::nullopt;
    }
    const auto value = trim(input);
    if (cancelled(value)) {
        return std::nullopt;
    }
    if (value.empty()) {
        return std::string {writer::defaults::task_line};
    }
    return std::string {value};
}

enum class NotesChoice {
    yes,
    no,
    cancelled
};

NotesChoice prompt_notes_directory(const std::string_view subdirectory) {
    while (true) {
        std::cout
            << "Create notes directory \""
            << subdirectory
            << "\"? [Y/n]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            return NotesChoice::cancelled;
        }
        const auto value = trim(input);
        if (cancelled(value)) {
            return NotesChoice::cancelled;
        }
        if (value.empty() || value == "y" || value == "Y") {
            return NotesChoice::yes;
        }
        if (value == "n" || value == "N") {
            return NotesChoice::no;
        }
        std::cout << "Enter Y or n.\n";
    }
}

std::optional<std::filesystem::path> path_from_setting(std::string_view text) {
    try {
        return std::filesystem::path {
            ac::encoding::to_utf16(std::string {text})
        };
    }
    catch (...) {
        writer_editor.log_print(
            "Failed to read config/writer.ini. Writer data files were not written."
        );
        return std::nullopt;
    }
}

std::optional<writer::data::Layout> load_layout() {
    const auto ini_path = ac::paths::config_directory() / "writer.ini";
    std::error_code error;
    const bool present = std::filesystem::exists(ini_path, error);
    if (error) {
        writer_editor.log_print(
            "Failed to inspect {}: {}",
            ini_path.string(),
            error.message()
        );
        return std::nullopt;
    }

    std::optional<std::filesystem::path> directory;
    std::optional<std::filesystem::path> notes;
    std::string notes_label;
    if (present) {
        const auto document = ac::ini::read(ini_path);
        if (!document) {
            writer_editor.log_print(
                "Failed to read {}. Writer data files were not written.",
                ini_path.string()
            );
            return std::nullopt;
        }
        if (const auto value = document->find("writer", "directory")) {
            const auto text = trim(*value);
            if (!text.empty()) {
                directory = path_from_setting(text);
                if (!directory) {
                    return std::nullopt;
                }
            }
        }
        if (const auto value = document->find("writer", "notes_subdirectory")) {
            const auto text = trim(*value);
            if (!text.empty()) {
                notes = path_from_setting(text);
                if (!notes) {
                    return std::nullopt;
                }
                notes_label = std::string {text};
                if (!writer::data::is_relative_subdirectory(*notes)) {
                    writer_editor.log_print(
                        "config/writer.ini has an invalid notes_subdirectory. "
                        "Using the notes folder under the Writer directory. "
                        "The file will not be rewritten."
                    );
                }
            }
        }
    }

    return writer::data::resolve_layout(
        directory,
        notes,
        notes_label,
        ac::paths::installation_root()
    );
}

int seed_session_prompts(const writer::data::Layout& layout) {
    return write_missing_file(
        layout.session_prompts,
        writer::defaults::session_prompts_text
    ) ? 0 : 1;
}

int seed_task_list(const writer::data::Layout& layout) {
    return write_missing_file(
        layout.task_list,
        writer::defaults::task_list_text
    ) ? 0 : 1;
}

int seed_notes(const writer::data::Layout& layout) {
    const auto state = inspect_store(layout.notes);
    if (state == StoreState::unreadable) {
        return 1;
    }
    if (state == StoreState::present) {
        writer_editor.log_print(
            "Left existing {} unchanged.",
            layout.notes.string()
        );
        return 0;
    }
    return create_notes_directory(layout.notes) ? 0 : 1;
}

int seed_stores(const writer::data::Layout& layout) {
    int failed = 0;
    if (seed_session_prompts(layout) != 0) {
        failed = 1;
    }
    if (seed_task_list(layout) != 0) {
        failed = 1;
    }
    if (seed_notes(layout) != 0) {
        failed = 1;
    }
    return failed;
}

int init_session_prompts(const writer::data::Layout& layout) {
    namespace req = ac::config::components_request;
    const auto state = inspect_store(layout.session_prompts);
    if (state == StoreState::unreadable) {
        return 1;
    }
    if (state == StoreState::present) {
        req::log_initialization_skipped(
            writer_editor,
            layout.session_prompts.string()
        );
        return 0;
    }

    const auto primary = prompt_recommended(
        writer::defaults::session_prompt_primary
    );
    if (!primary) {
        return 1;
    }
    const auto secondary = prompt_recommended(
        writer::defaults::session_prompt_secondary
    );
    if (!secondary) {
        return 1;
    }
    const std::string contents = *primary + "\n" + *secondary + "\n";
    return write_missing_file(layout.session_prompts, contents) ? 0 : 1;
}

int init_task_list(const writer::data::Layout& layout) {
    namespace req = ac::config::components_request;
    const auto state = inspect_store(layout.task_list);
    if (state == StoreState::unreadable) {
        return 1;
    }
    if (state == StoreState::present) {
        req::log_initialization_skipped(writer_editor, layout.task_list.string());
        return 0;
    }

    const auto task = prompt_task();
    if (!task) {
        return 1;
    }
    return write_missing_file(layout.task_list, *task + "\n") ? 0 : 1;
}

int init_notes(const writer::data::Layout& layout) {
    namespace req = ac::config::components_request;
    const auto state = inspect_store(layout.notes);
    if (state == StoreState::unreadable) {
        return 1;
    }
    if (state == StoreState::present) {
        req::log_initialization_skipped(writer_editor, layout.notes.string());
        return 0;
    }

    switch (prompt_notes_directory(layout.notes_subdirectory)) {
    case NotesChoice::yes:
        return create_notes_directory(layout.notes) ? 0 : 1;
    case NotesChoice::no:
        writer_editor.log_print("Notes directory was not requested.");
        return 0;
    case NotesChoice::cancelled:
        return 1;
    }
    return 1;
}

int init_stores(const writer::data::Layout& layout) {
    if (init_session_prompts(layout) != 0) {
        return 1;
    }
    if (init_task_list(layout) != 0) {
        return 1;
    }
    return init_notes(layout);
}

std::optional<ac::config::components_request::ConfigLaunch>
initialization_launch(const int argc, char* argv[]) {
    if (argc < 2) {
        return std::nullopt;
    }
    ac::config::components_request::ConfigLaunch launch;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument {argv[index]};
        if (argument == ac::config::components_request::init_flag ||
            argument == ac::config::components_request::initialize_flag) {
            launch.init = true;
            continue;
        }
        if (argument == ac::config::components_request::seed_flag) {
            launch.seed = true;
            continue;
        }
        return std::nullopt;
    }
    if (!launch.init && !launch.seed) {
        return std::nullopt;
    }
    return launch;
}

enum class Action {
    menu,
    add_task,
    add_session_prompt,
    daily_note,
};

std::optional<Action> parse_action(int argc, char* argv[]) {
    if (argc == 1) {
        return Action::menu;
    }
    if (argc != 2) {
        return std::nullopt;
    }
    const std::string_view argument {argv[1]};
    if (argument == "--add-task") {
        return Action::add_task;
    }
    if (argument == "--add-session-prompt") {
        return Action::add_session_prompt;
    }
    if (argument == "--daily-note") {
        return Action::daily_note;
    }
    return std::nullopt;
}

bool requires_initialization(Action action) {
    return action == Action::menu ||
        action == Action::add_task ||
        action == Action::add_session_prompt ||
        action == Action::daily_note;
}

void print_menu() {
    std::cout
        << "\nwriter_editor\n"
        << "  1. Add new session prompt\n"
        << "  2. Add task\n"
        << "  3. Create new note\n"
        << "  4. Exit\n"
        << "> ";
}

int run_menu() {
    while (true) {
        print_menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        if (choice == "1") {
            (void)add_session_prompt();
        }
        else if (choice == "2") {
            (void)add_task();
        }
        else if (choice == "3") {
            (void)open_daily_note();
        }
        else if (choice == "4" || choice == "q" || choice == "Q") {
            return 0;
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}

int run_action(Action action) {
    switch (action) {
        case Action::menu:
            return run_menu();
        case Action::add_task:
            return add_task();
        case Action::add_session_prompt:
            return add_session_prompt();
        case Action::daily_note:
            return open_daily_note();
    }
    return 1;
}

} // namespace

int main(int argc, char* argv[]) {
    std::setvbuf(stdin, nullptr, _IONBF, 0);
    ac::shell::set_process_app_user_model_id();
    writer_editor.log_main("writer_editor.exe started");

    if (const auto launch = initialization_launch(argc, argv)) {
        const auto layout = load_layout();
        if (!layout) {
            return 1;
        }
        namespace req = ac::config::components_request;
        req::log_config_request(writer_editor, *launch);
        if (launch->seed) {
            return seed_stores(*layout);
        }
        return init_stores(*layout);
    }

    const auto action = parse_action(argc, argv);
    if (!action) {
        writer_editor.log_print("Unknown writer_editor argument.");
        return 1;
    }
    if (requires_initialization(*action) && !require_initialization()) {
        return 1;
    }
    return run_action(*action);
}
