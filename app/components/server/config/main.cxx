#include <Windows.h>
#include <shellapi.h>

import std;
import auto_core.core.component;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;
import server_defaults;
import components_editor_request;

import <iostream>;

namespace {

ac::Component server_config {"server_config"};

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
    return server::defaults::port;
}

std::string stored_document_root(const ac::ini::Document& document) {
    const auto value = document.find("server", "document_root");
    if (!value || value->empty()) {
        return std::string {server::defaults::document_root};
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
    return server::defaults::port;
}

std::string current_stored_document_root() {
    if (const auto document = read_server_ini()) {
        return stored_document_root(*document);
    }
    return std::string {server::defaults::document_root};
}

std::filesystem::path resolved_document_root(std::string_view stored) {
    std::filesystem::path configured {
        ac::encoding::to_utf16(stored)
    };
    if (configured.is_relative()) {
        configured = ac::paths::installation_root() / configured;
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

bool write_server_ini(std::string_view document_root, int port) {
    std::error_code error;
    std::filesystem::create_directories(ac::paths::config_directory(), error);
    if (error) {
        server_config.log_print(
            "Failed to create config directory: {}",
            error.message()
        );
        return false;
    }

    const auto path = ac::paths::config_directory() / "server.ini";
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        server_config.log_print("Failed to create {}", path.string());
        return false;
    }
    const auto contents = server::defaults::ini_for(document_root, port);
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    if (!output) {
        server_config.log_print("Failed to write {}", path.string());
        return false;
    }
    return true;
}

bool ensure_server_ini(const bool prompt) {
    const auto path = ac::paths::config_directory() / "server.ini";

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        return true;
    }
    if (error) {
        server_config.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return false;
    }

    if (!prompt) {
        return write_server_ini(
            std::string {server::defaults::document_root},
            server::defaults::port
        );
    }

    const auto directory = prompt_document_root(".\\components\\server", server::defaults::document_root);
    if (!directory) {
        return false;
    }
    const auto port = prompt_port(server::defaults::port);
    if (!port) {
        return false;
    }

    return write_server_ini(*directory, *port);
}

void show_port() {
    std::cout << "Port: " << current_port() << '\n';
}

void set_port() {
    const auto port = prompt_port(current_port());
    if (!port) {
        return;
    }
    if (!write_server_ini(current_stored_document_root(), *port)) {
        server_config.log_print("Failed to write config/server.ini port.");
        return;
    }
    server_config.log_print("Port stored as {}.", *port);
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
    if (!write_server_ini(*directory, current_port())) {
        server_config.log_print(
            "Failed to write config/server.ini document_root."
        );
        return;
    }
    server_config.log_print("document_root stored as {}.", *directory);
}

void open_folder(const std::filesystem::path& directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        server_config.log_print(
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
        server_config.log_print("Failed to open {}", directory.string());
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

int main(int argc, char* argv[]) {
    server_config.log_main("server_config.exe started");

    const bool initialize =
        ac::config::components_request::is_initialize_run(argc, argv);
    if (!ensure_server_ini(!initialize)) {
        server_config.log_print("Server configuration was not initialized.");
        return 1;
    }

    if (initialize) {
        server_config.log_main("server.ini initialized");
        return ac::config::components_request::run_component_update("server");
    }

    activate_own_console();
    show_port();
    show_document_root();

    while (true) {
        print_menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return ac::config::components_request::run_component_update("server");
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
            return ac::config::components_request::run_component_update("server");
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}
