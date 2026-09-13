#include <Windows.h>
#include <shellapi.h>

import std;
import auto_core.core.config;
import auto_core.core.encoding;
import auto_core.core.ini;
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

std::optional<int> parse_port(std::string_view value) {
    int parsed_port = 0;
    const auto result = std::from_chars(
        value.data(),
        value.data() + value.size(),
        parsed_port
    );
    if (result.ec == std::errc {} &&
        result.ptr == value.data() + value.size() &&
        parsed_port >= 1 &&
        parsed_port <= 65535) {
        return parsed_port;
    }
    return std::nullopt;
}

int read_port(const ac::ini::Document& document) {
    if (const auto value = document.find("server", "port")) {
        if (const auto parsed = parse_port(*value)) {
            return *parsed;
        }
    }
    return 8585;
}

std::string stored_document_root(const ac::ini::Document& document) {
    const auto value = document.find("server", "document_root");
    if (!value || value->empty()) {
        return "server";
    }
    return std::string {*value};
}

std::optional<ac::ini::Document> read_server_ini() {
    return ac::ini::read(ac::paths::config_directory() / "server.ini");
}

int current_port() {
    if (const auto document = read_server_ini()) {
        return read_port(*document);
    }
    return 8585;
}

std::string current_stored_document_root() {
    if (const auto document = read_server_ini()) {
        return stored_document_root(*document);
    }
    return "server";
}

std::filesystem::path resolved_document_root(std::string_view stored) {
    std::filesystem::path configured {
        ac::encoding::to_utf16(stored)
    };
    if (configured.is_relative()) {
        configured = ac::paths::executable_directory() / configured;
    }
    return configured.lexically_normal();
}

std::optional<int> prompt_port(int default_port) {
    while (true) {
        std::cout << "Port [" << default_port << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            return std::nullopt;
        }

        const auto value = trim(input);
        if (value.empty()) {
            return default_port;
        }
        if (const auto parsed = parse_port(value)) {
            return parsed;
        }
        std::cout
            << "Enter an integer from 1 to 65535, or leave blank for "
            << default_port
            << ".\n";
    }
}

std::optional<std::string> prompt_document_root(
    std::string_view displayed_default,
    std::string_view stored_default
) {
    std::cout << "Server document root [" << displayed_default << "]: ";
    std::string input;
    if (!std::getline(std::cin, input)) {
        return std::nullopt;
    }

    const auto directory = trim(input);
    if (directory.empty()) {
        return std::string {stored_default};
    }
    return std::string {directory};
}

bool ensure_server_ini() {
    const auto path = ac::paths::config_directory() / "server.ini";

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

    const auto directory = prompt_document_root(".\\server", "server");
    if (!directory) {
        return false;
    }
    const auto port = prompt_port(8585);
    if (!port) {
        return false;
    }

    return ac::config::write_server_ini_if_missing(*directory, *port);
}

void show_port() {
    std::cout << "Port: " << current_port() << '\n';
}

void set_port() {
    const auto port = prompt_port(current_port());
    if (!port) {
        return;
    }
    if (!ac::config::write_server_ini_port(*port)) {
        std::cerr << "Failed to write config/server.ini port.\n";
        return;
    }
    std::cout << "Port stored as " << *port << ".\n";
}

void show_document_root() {
    const auto stored = current_stored_document_root();
    std::cout
        << "document_root: " << stored << '\n'
        << "Resolved path: " << resolved_document_root(stored).string()
        << '\n';
}

void set_document_root() {
    const auto stored = current_stored_document_root();
    const auto directory = prompt_document_root(stored, stored);
    if (!directory) {
        return;
    }
    if (!ac::config::write_server_ini_document_root(*directory)) {
        std::cerr << "Failed to write config/server.ini document_root.\n";
        return;
    }
    std::cout << "document_root stored as " << *directory << ".\n";
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
        << "\nserver_config\n"
        << "  1. Show port\n"
        << "  2. Set port\n"
        << "  3. Show document_root\n"
        << "  4. Set document_root\n"
        << "  5. Open document_root folder\n"
        << "  6. Exit\n"
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
    if (!ensure_server_ini()) {
        return 1;
    }

    ac::config::initialize_core_settings();

    activate_own_console();
    show_port();
    show_document_root();

    while (true) {
        print_menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        if (choice == "1") {
            show_port();
        }
        else if (choice == "2") {
            set_port();
        }
        else if (choice == "3") {
            show_document_root();
        }
        else if (choice == "4") {
            set_document_root();
        }
        else if (choice == "5") {
            open_folder(
                resolved_document_root(current_stored_document_root())
            );
        }
        else if (choice == "6" || choice == "q" || choice == "Q") {
            return 0;
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}
