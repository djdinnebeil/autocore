#include <Windows.h>
#include <shellapi.h>

import std;
import auto_core.core.config;
import auto_core.core.paths;

import <iostream>;

namespace {

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool ensure_writer_ini() {
    const auto path = ac::paths::config_directory() / "writer.ini";

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        return true;
    }
    if (error) {
        std::cerr
            << "Failed to inspect "
            << path.string()
            << ": "
            << error.message()
            << '\n';
        return false;
    }

    std::cout << "Writer data directory [.\\writer]: ";
    std::string input;
    std::getline(std::cin, input);

    const auto directory = trim(input);
    return ac::config::write_writer_ini_if_missing(
        directory.empty() ? "writer" : directory,
        "notes"
    );
}

std::filesystem::path gpt_prompts_file() {
    return ac::paths::writer_directory() / "gpt_prompts.txt";
}

std::filesystem::path task_list_file() {
    return ac::paths::writer_directory() / "task_list.txt";
}

const char* exists_label(const std::filesystem::path& path) {
    std::error_code error;
    if (std::filesystem::exists(path, error) && !error) {
        return "exists";
    }
    return "missing";
}

void show_paths() {
    const auto prompts = gpt_prompts_file();
    const auto tasks = task_list_file();
    std::cout
        << "Writer directory: " << ac::paths::writer_directory().string()
        << '\n'
        << "Notes directory: " << ac::paths::notes_directory().string()
        << '\n'
        << "gpt_prompts.txt: " << prompts.string() << " ("
        << exists_label(prompts) << ")\n"
        << "task_list.txt: " << tasks.string() << " ("
        << exists_label(tasks) << ")\n";
}

bool write_empty_if_missing(const std::filesystem::path& path) {
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        std::cout << "Already exists: " << path.string() << '\n';
        return true;
    }
    if (error) {
        std::cerr
            << "Failed to inspect "
            << path.string()
            << ": "
            << error.message()
            << '\n';
        return false;
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        std::cerr << "Failed to create " << path.string() << '\n';
        return false;
    }
    std::cout << "Created " << path.string() << '\n';
    return true;
}

void create_missing_writer_files() {
    std::error_code error;
    std::filesystem::create_directories(ac::paths::writer_directory(), error);
    if (error) {
        std::cerr
            << "Failed to create writer directory: "
            << error.message()
            << '\n';
        return;
    }

    (void)write_empty_if_missing(gpt_prompts_file());
    (void)write_empty_if_missing(task_list_file());
}

void open_folder(const std::filesystem::path& directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        std::cerr
            << "Failed to create "
            << directory.string()
            << ": "
            << error.message()
            << '\n';
        return;
    }

    const HINSTANCE result = ShellExecuteW(
        nullptr,
        L"open",
        directory.c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );
    if (reinterpret_cast<std::intptr_t>(result) <= 32) {
        std::cerr
            << "Failed to open "
            << directory.string()
            << '\n';
    }
}

void print_menu() {
    std::cout
        << "\nwriter_config\n"
        << "  1. Show paths\n"
        << "  2. Create missing gpt_prompts.txt and task_list.txt\n"
        << "  3. Open writer folder\n"
        << "  4. Open notes folder\n"
        << "  5. Exit\n"
        << "> ";
}

void activate_own_console() {
    const HWND console = GetConsoleWindow();
    if (console == nullptr) {
        return;
    }
    if (IsIconic(console)) {
        (void)ShowWindow(console, SW_RESTORE);
    }
    (void)BringWindowToTop(console);
    (void)SetForegroundWindow(console);
    (void)SetFocus(console);
}

} // namespace

int main() {
    if (!ensure_writer_ini()) {
        return 1;
    }

    ac::config::initialize_core_settings();

    activate_own_console();
    show_paths();

    while (true) {
        print_menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        if (choice == "1") {
            show_paths();
        }
        else if (choice == "2") {
            create_missing_writer_files();
        }
        else if (choice == "3") {
            open_folder(ac::paths::writer_directory());
        }
        else if (choice == "4") {
            open_folder(ac::paths::notes_directory());
        }
        else if (choice == "5" || choice == "q" || choice == "Q") {
            return 0;
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}
