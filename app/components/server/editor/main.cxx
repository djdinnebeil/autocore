#include <Windows.h>
#include <shellapi.h>

#include "../shared/server_data_detail.hpp"

import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import server_defaults;
import components_editor_request;

import <iostream>;
import auto_core.core.shell;

namespace {

ac::Component server_editor {
    "server_editor",
    ac::logging::config::LoggingScope {"server"}
};

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

enum class DirectoryLoad {
    ready,
    unreadable
};

struct DataDirectory {
    DirectoryLoad load {DirectoryLoad::unreadable};
    std::filesystem::path path;
};

DataDirectory load_data_directory() {
    const auto ini_path = ac::paths::config_directory() / "server.ini";
    std::error_code error;
    const bool present = std::filesystem::exists(ini_path, error);
    if (error) {
        server_editor.log_print(
            "Failed to inspect {}: {}",
            ini_path.string(),
            error.message()
        );
        return {};
    }
    if (!present) {
        return {
            DirectoryLoad::ready,
            server::data::resolve_directory(
                std::nullopt,
                ac::paths::installation_root()
            )
        };
    }

    const auto document = ac::ini::read(ini_path);
    if (!document) {
        server_editor.log_print(
            "Failed to read {}. Server data files were not written.",
            ini_path.string()
        );
        return {};
    }

    std::optional<std::string> stored;
    if (const auto value = document->find("server", "directory")) {
        const auto text = trim(*value);
        if (!text.empty()) {
            stored = std::string {text};
        }
    }
    std::optional<std::string_view> view;
    if (stored) {
        view = *stored;
    }
    return {
        DirectoryLoad::ready,
        server::data::resolve_directory(view, ac::paths::installation_root())
    };
}

enum class ScalarFile {
    missing,
    unreadable,
    ok
};

struct ScalarText {
    ScalarFile status {ScalarFile::missing};
    std::string text;
};

ScalarText read_scalar(const std::filesystem::path& path) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        return {ScalarFile::unreadable, {}};
    }
    if (!present) {
        return {ScalarFile::missing, {}};
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {ScalarFile::unreadable, {}};
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    return {ScalarFile::ok, contents.str()};
}

bool write_new_file(
    const std::filesystem::path& path,
    const std::string_view contents
) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        server_editor.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return false;
    }
    if (present) {
        server_editor.log_print("Left existing {} unchanged.", path.string());
        return true;
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        server_editor.log_print("Failed to create {}", path.string());
        return false;
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    if (!output) {
        server_editor.log_print("Failed to write {}", path.string());
        return false;
    }
    server_editor.log_print("Created {}", path.string());
    return true;
}

bool replace_scalar(
    const std::filesystem::path& path,
    const std::string_view contents
) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        server_editor.log_print(
            "Failed to create {}: {}",
            path.parent_path().string(),
            error.message()
        );
        return false;
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        server_editor.log_print("Failed to create {}", path.string());
        return false;
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    if (!output) {
        server_editor.log_print("Failed to write {}", path.string());
        return false;
    }
    return true;
}

std::filesystem::path port_path(const std::filesystem::path& directory) {
    return directory / "port.id";
}

std::filesystem::path document_root_path(const std::filesystem::path& directory) {
    return directory / "document_root.id";
}

bool cancelled(const std::string_view value) {
    if (value != "cancel") {
        return false;
    }
    server_editor.log_print("Cancelled.");
    return true;
}

std::optional<int> prompt_init_port() {
    const int recommended = server::defaults::port;
    while (true) {
        std::cout << "Server port [" << recommended << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            return std::nullopt;
        }
        const auto value = trim(input);
        if (cancelled(value)) {
            return std::nullopt;
        }
        if (value.empty()) {
            return recommended;
        }
        if (const auto parsed = server::data::parse_port(value)) {
            return parsed;
        }
        std::cout
            << "Enter an integer from 1 to 65535, or leave blank for "
            << recommended
            << ".\n";
    }
}

std::optional<std::string> prompt_init_document_root() {
    const std::string recommended {server::defaults::document_root};
    while (true) {
        std::cout << "Document root [" << recommended << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            return std::nullopt;
        }
        const auto value = trim(input);
        if (cancelled(value)) {
            return std::nullopt;
        }
        const std::string stored = value.empty() ? recommended : std::string {value};
        if (server::data::parse_document_root(stored)) {
            return stored;
        }
        std::cout << "Enter one document-root path.\n";
    }
}

int ensure_data_directory(const std::filesystem::path& directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        server_editor.log_print(
            "Failed to create {}: {}",
            directory.string(),
            error.message()
        );
        return 1;
    }
    return 0;
}

int init_files(const std::filesystem::path& directory) {
    if (const int created = ensure_data_directory(directory); created != 0) {
        return created;
    }

    namespace req = ac::config::components_request;
    const auto port = port_path(directory);
    const auto port_text = read_scalar(port);
    if (port_text.status == ScalarFile::unreadable) {
        server_editor.log_print("Failed to read {}", port.string());
        return 1;
    }
    if (port_text.status == ScalarFile::ok) {
        req::log_initialization_skipped(server_editor, port.string());
    }
    else {
        const auto chosen = prompt_init_port();
        if (!chosen) {
            return 1;
        }
        if (!write_new_file(port, std::to_string(*chosen) + "\n")) {
            return 1;
        }
    }

    const auto root = document_root_path(directory);
    const auto root_text = read_scalar(root);
    if (root_text.status == ScalarFile::unreadable) {
        server_editor.log_print("Failed to read {}", root.string());
        return 1;
    }
    if (root_text.status == ScalarFile::ok) {
        req::log_initialization_skipped(server_editor, root.string());
        return 0;
    }

    const auto chosen = prompt_init_document_root();
    if (!chosen) {
        return 1;
    }
    if (!write_new_file(root, *chosen + "\n")) {
        return 1;
    }
    return 0;
}

int seed_files(const std::filesystem::path& directory) {
    if (const int created = ensure_data_directory(directory); created != 0) {
        return created;
    }

    const std::string port_text = std::to_string(server::defaults::port) + "\n";
    const std::string root_text = std::string {server::defaults::document_root} + "\n";
    const bool port_written = write_new_file(port_path(directory), port_text);
    const bool root_written = write_new_file(document_root_path(directory), root_text);
    return port_written && root_written ? 0 : 1;
}

void show_port(const std::filesystem::path& directory) {
    const auto text = read_scalar(port_path(directory));
    const auto parsed = text.status == ScalarFile::ok
        ? server::data::parse_port(text.text)
        : std::nullopt;
    if (!parsed) {
        std::cout << "port.id is missing or invalid.\n";
        return;
    }
    std::cout << "port: " << *parsed << '\n';
}

void show_document_root(const std::filesystem::path& directory) {
    const auto text = read_scalar(document_root_path(directory));
    const auto parsed = text.status == ScalarFile::ok
        ? server::data::parse_document_root(text.text)
        : std::nullopt;
    if (!parsed) {
        std::cout << "document_root.id is missing or invalid.\n";
        return;
    }
    const auto resolved = server::data::resolve_document_root(*parsed, directory);
    std::cout
        << "document_root: " << *parsed << '\n'
        << "Resolved path: " << resolved.string() << '\n';
}

std::optional<int> prompt_port(int displayed) {
    while (true) {
        std::cout << "Port [" << displayed << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            return std::nullopt;
        }
        const auto value = trim(input);
        if (value.empty()) {
            return displayed;
        }
        if (const auto parsed = server::data::parse_port(value)) {
            return parsed;
        }
        std::cout
            << "Enter an integer from 1 to 65535, or leave blank for "
            << displayed
            << ".\n";
    }
}

void set_port(const std::filesystem::path& directory) {
    const auto text = read_scalar(port_path(directory));
    const auto current = text.status == ScalarFile::ok
        ? server::data::parse_port(text.text)
        : std::nullopt;
    const auto port = prompt_port(current.value_or(server::defaults::port));
    if (!port) {
        return;
    }
    const auto contents = std::to_string(*port) + "\n";
    if (!replace_scalar(port_path(directory), contents)) {
        server_editor.log_print("Failed to write port.id.");
        return;
    }
    server_editor.log_print("Port stored as {}.", *port);
}

void set_document_root(const std::filesystem::path& directory) {
    const auto text = read_scalar(document_root_path(directory));
    const auto current = text.status == ScalarFile::ok
        ? server::data::parse_document_root(text.text)
        : std::nullopt;
    const std::string displayed = current.value_or(
        std::string {server::defaults::document_root}
    );
    std::cout << "Document root [" << displayed << "]: ";
    std::string input;
    if (!std::getline(std::cin, input)) {
        return;
    }
    const auto chosen = trim(input);
    const auto stored = chosen.empty() ? displayed : std::string {chosen};
    if (!server::data::parse_document_root(stored)) {
        std::cout << "Enter one document-root path.\n";
        return;
    }
    if (!replace_scalar(document_root_path(directory), stored + "\n")) {
        server_editor.log_print("Failed to write document_root.id.");
        return;
    }
    server_editor.log_print("document_root stored as {}.", stored);
}

void open_folder(const std::filesystem::path& directory) {
    const auto text = read_scalar(document_root_path(directory));
    const auto stored = text.status == ScalarFile::ok
        ? server::data::parse_document_root(text.text)
        : std::nullopt;
    if (!stored) {
        server_editor.log_print(
            "Server document_root.id is missing or invalid. Run server_editor.exe to configure Server."
        );
        return;
    }

    const auto root = server::data::resolve_document_root(*stored, directory);
    std::error_code error;
    std::filesystem::create_directories(root, error);
    if (error) {
        server_editor.log_print(
            "Failed to create {}: {}",
            root.string(),
            error.message()
        );
        return;
    }

    const HINSTANCE result = ShellExecuteW(
        nullptr,
        L"open",
        root.c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );
    if (reinterpret_cast<std::intptr_t>(result) <= 32) {
        server_editor.log_print("Failed to open {}", root.string());
    }
}

void print_menu() {
    std::cout
        << "\nserver_editor\n"
        << "  1. Show port\n"
        << "  2. Set port\n"
        << "  3. Show document root\n"
        << "  4. Set document root\n"
        << "  5. Open document root folder\n"
        << "  6. Exit\n"
        << "> ";
}

int run_menu(const std::filesystem::path& directory) {
    show_port(directory);
    show_document_root(directory);
    while (true) {
        print_menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        if (choice == "1") {
            show_port(directory);
        }
        else if (choice == "2") {
            set_port(directory);
        }
        else if (choice == "3") {
            show_document_root(directory);
        }
        else if (choice == "4") {
            set_document_root(directory);
        }
        else if (choice == "5") {
            open_folder(directory);
        }
        else if (choice == "6" || choice == "q" || choice == "Q") {
            return 0;
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::setvbuf(stdin, nullptr, _IONBF, 0);
    ac::shell::set_process_app_user_model_id();
    server_editor.log_main("server_editor.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        std::cerr << "Usage: server_editor.exe [--init | --seed]\n";
        return 1;
    }

    const auto directory = load_data_directory();
    if (directory.load != DirectoryLoad::ready) {
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(server_editor, *launch);
    if (launch->seed) {
        return seed_files(directory.path);
    }
    if (launch->init) {
        return init_files(directory.path);
    }
    return run_menu(directory.path);
}
