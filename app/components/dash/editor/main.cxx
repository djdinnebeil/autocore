#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

import auto_core.core.component;
import auto_core.core.logging.config;
import dash_hello;
import dash_vault;
import dash_vault_mutate;
import auto_core.core.shell;

namespace {

constexpr wchar_t editor_mutex_name[] = L"Local\\AutoCoreDashEditor";

std::optional<std::size_t> read_selection(std::size_t maximum) {
    std::wstring line;
    std::getline(std::wcin, line);
    try {
        std::size_t used{};
        const auto number = std::stoull(line, &used);
        if (used == line.size() && number >= 1 && number <= maximum) {
            return static_cast<std::size_t>(number - 1);
        }
    } catch (...) {
    }
    return std::nullopt;
}

std::wstring read_hidden_line() {
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD original_mode{};
    if (input == INVALID_HANDLE_VALUE || !GetConsoleMode(input, &original_mode)) {
        throw std::runtime_error("Secure console input is unavailable.");
    }

    if (!SetConsoleMode(input, original_mode & ~ENABLE_ECHO_INPUT)) {
        throw std::runtime_error("Unable to disable console echo.");
    }

    std::wstring value;
    try {
        std::getline(std::wcin, value);
    } catch (...) {
        SetConsoleMode(input, original_mode);
        throw;
    }
    SetConsoleMode(input, original_mode);
    std::wcout << L'\n';
    return value;
}

void print_secrets(const std::vector<dash::vault::SecretRecord>& records) {
    std::wcout << L"\nStored secrets:\n";
    for (std::size_t index = 0; index < records.size(); ++index) {
        std::wcout << index + 1 << L". " << records[index].name << L'\n';
    }
}

void add_secret(const std::filesystem::path& path) {
    auto records = dash::vault::load_vault(path);
    if (records.size() >= dash::vault::maximum_entry_count) {
        throw std::runtime_error("The dash vault has reached its entry limit.");
    }

    std::wcout << L"Secret name: ";
    std::wstring name;
    std::getline(std::wcin, name);
    if (name.empty() || name.size() > dash::vault::maximum_name_characters ||
        std::ranges::any_of(name, [](wchar_t character) {
            return std::iswcntrl(character) != 0;
        })) {
        throw std::runtime_error(
            "The secret name is empty, too long, or contains a control character.");
    }
    if (std::ranges::any_of(records, [&name](const dash::vault::SecretRecord& record) {
            return _wcsicmp(record.name.c_str(), name.c_str()) == 0;
        })) {
        throw std::runtime_error("A secret with that name already exists.");
    }

    std::wcout << L"Secret value (input hidden): ";
    std::wstring value = read_hidden_line();
    if (value.empty()) {
        throw std::runtime_error("The secret value cannot be empty.");
    }

    try {
        records.push_back({std::move(name), dash::vault::protect(value)});
        dash::vault::wipe(value);
        dash::vault::save_vault(path, records);
    } catch (...) {
        dash::vault::wipe(value);
        throw;
    }
    std::wcout << L"Secret stored with current-user DPAPI.\n";
}

void list_secrets(const std::filesystem::path& path) {
    const auto records = dash::vault::load_vault(path);
    if (records.empty()) {
        std::wcout << L"No secrets have been stored.\n";
        return;
    }
    print_secrets(records);
}

void remove_secret(const std::filesystem::path& path) {
    auto records = dash::vault::load_vault(path);
    if (records.empty()) {
        std::wcout << L"No secrets have been stored.\n";
        return;
    }
    print_secrets(records);
    std::wcout << L"Select a secret to remove: ";
    const auto selection = read_selection(records.size());
    if (!selection) {
        throw std::runtime_error("Invalid secret selection.");
    }
    if (!dash::hello::verify_user()) {
        std::wcout << L"Windows Hello verification was not completed.\n";
        return;
    }
    std::wcout << L"Remove '" << records[*selection].name
               << L"'? Type yes to confirm: ";
    std::wstring confirmation;
    std::getline(std::wcin, confirmation);
    if (confirmation != L"yes") {
        std::wcout << L"Secret was not removed.\n";
        return;
    }
    dash::vault::wipe(records[*selection].protected_value);
    records.erase(records.begin() +
        static_cast<std::ptrdiff_t>(*selection));
    dash::vault::save_vault(path, records);
    std::wcout << L"Secret removed.\n";
}

} // namespace

int wmain() {
    ac::shell::set_process_app_user_model_id();
    // Held until process exit. A second editor must not open the vault.
    const HANDLE editor_mutex = CreateMutexW(nullptr, FALSE, editor_mutex_name);
    if (editor_mutex == nullptr) {
        std::cerr << "dash_editor failed: Unable to create the Dash editor lock.\n";
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(editor_mutex);
        std::wcerr << L"Dash secret editor is already running.\n";
        return 1;
    }

    try {
        dash::hello::initialize();
        SetConsoleTitleW(L"Auto Core Dash Editor");
        ac::Component dash_editor {
            "dash_editor",
            ac::logging::config::LoggingScope {
                "dash",
                ac::logging::config::LoggingFallback::off
            }
        };
        dash_editor.log_main("dash_editor.exe started");

        while (true) {
            std::wcout << L"\ndash - Secret Editor\n\n"
                       << L"1. Add a secret\n"
                       << L"2. Remove a secret\n"
                       << L"3. List secrets\n"
                       << L"4. Exit\n\n"
                       << L"Select an option: ";

            const auto selection = read_selection(4);
            if (!selection) {
                std::wcerr << L"Invalid menu option.\n";
                continue;
            }
            try {
                switch (*selection) {
                case 0: add_secret(dash::vault::vault_path()); break;
                case 1: remove_secret(dash::vault::vault_path()); break;
                case 2: list_secrets(dash::vault::vault_path()); break;
                case 3: return 0;
                default: break;
                }
            } catch (const std::exception& error) {
                std::cerr << "dash_editor: " << error.what() << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "dash_editor failed: " << error.what() << '\n';
        return 1;
    }
}
