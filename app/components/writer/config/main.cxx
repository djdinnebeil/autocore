#include <Windows.h>
#include <shellapi.h>

import std;
import auto_core.core.component;
import auto_core.core.paths;
import writer_defaults;
import components_editor_request;

import <iostream>;

namespace {

ac::Component writer_config {"writer_config"};

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool ensure_writer_ini(const bool prompt) {
    const auto path = ac::paths::config_directory() / "writer.ini";

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        return true;
    }
    if (error) {
        writer_config.log_and_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return false;
    }

    std::string directory;
    if (prompt) {
        std::cout << "Writer data directory [.\\writer]: ";
        std::string input;
        std::getline(std::cin, input);
        directory = std::string {trim(input)};
    }

    std::error_code create_error;
    std::filesystem::create_directories(ac::paths::config_directory(), create_error);
    if (create_error) {
        writer_config.log_and_print(
            "Failed to create config directory: {}",
            create_error.message()
        );
        return false;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        writer_config.log_and_print("Failed to create {}", path.string());
        return false;
    }
    const auto contents = writer::defaults::ini_for(
        directory,
        writer::defaults::notes_directory
    );
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    return static_cast<bool>(output);
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
        writer_config.log_and_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return false;
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        writer_config.log_and_print("Failed to create {}", path.string());
        return false;
    }
    writer_config.log_and_print("Created {}", path.string());
    return true;
}

void create_missing_writer_files() {
    std::error_code error;
    std::filesystem::create_directories(ac::paths::writer_directory(), error);
    if (error) {
        writer_config.log_and_print(
            "Failed to create writer directory: {}",
            error.message()
        );
        return;
    }

    (void)write_empty_if_missing(gpt_prompts_file());
    (void)write_empty_if_missing(task_list_file());
}

void open_folder(const std::filesystem::path& directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        writer_config.log_and_print(
            "Failed to create {}: {}",
            directory.string(),
            error.message()
        );
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
        writer_config.log_and_print("Failed to open {}", directory.string());
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

int main(int argc, char* argv[]) {
    writer_config.connect_to_logger();
    writer_config.log_and_log("writer_config.exe started");

    const bool initialize =
        ac::config::components_request::is_initialize_run(argc, argv);
    if (!ensure_writer_ini(!initialize)) {
        writer_config.log_and_print("Writer configuration was not initialized.");
        return 1;
    }

    if (initialize) {
        writer_config.log_and_log("writer.ini initialized");
        return ac::config::components_request::run_component_update("writer");
    }

    activate_own_console();
    show_paths();

    while (true) {
        print_menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return ac::config::components_request::run_component_update("writer");
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
            return ac::config::components_request::run_component_update("writer");
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}
