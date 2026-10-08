#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "../../../main/shared/launch_descriptor_resource.hpp"

import auto_core.core.console;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import dash_vault;
import dash_vault_decrypt;
import dash_hello;
import auto_core.core.shell;

namespace {

std::mutex secret_prompt_mutex;
std::atomic_uintptr_t destination_window{};
constexpr wchar_t dash_window_title[] = L"Auto Core Dash";
constexpr wchar_t dash_mutex_name[] = L"Local\\AutoCoreDashInstance";
constexpr wchar_t dash_mapping_name[] = L"Local\\AutoCoreDashTarget";
constexpr wchar_t dash_event_name[] = L"Local\\AutoCoreDashActivate";

struct SharedTarget {
    std::uintptr_t window{};
    std::uintptr_t owner_console{};
};

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

std::optional<std::uint64_t> argument_number(
    int argument_count, wchar_t* arguments[], std::wstring_view name) {
    for (int index = 1; index + 1 < argument_count; ++index) {
        if (std::wstring_view {arguments[index]} != name) {
            continue;
        }
        try {
            std::size_t used{};
            const std::wstring_view value {arguments[index + 1]};
            const auto number = std::stoull(std::wstring {value}, &used);
            if (used == value.size()) {
                return number;
            }
        } catch (...) {
        }
    }
    return std::nullopt;
}

bool is_usable_destination(HWND window) {
    if (window == nullptr || !IsWindow(window) ||
        window == GetConsoleWindow()) {
        return false;
    }
    DWORD process_id{};
    GetWindowThreadProcessId(window, &process_id);
    return process_id != GetCurrentProcessId();
}

HWND acquire_destination() {
    HWND window = reinterpret_cast<HWND>(destination_window.load());
    if (is_usable_destination(window)) {
        return window;
    }

    std::wcout << L"Switch to the destination text field...\n";
    const HWND console = GetConsoleWindow();
    ShowWindow(console, SW_MINIMIZE);
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds {30};
    while (std::chrono::steady_clock::now() < deadline) {
        window = GetForegroundWindow();
        if (is_usable_destination(window)) {
            destination_window.store(
                reinterpret_cast<std::uintptr_t>(window));
            (void)ac::console::activate();
            return window;
        }
        Sleep(25);
    }
    (void)ac::console::activate();
    return nullptr;
}

bool initialize_single_instance(std::uintptr_t initial_target) {
    const HANDLE instance_mutex = CreateMutexW(
        nullptr, FALSE, dash_mutex_name);
    if (instance_mutex == nullptr) {
        throw std::runtime_error("Unable to create the Dash instance lock.");
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        const HANDLE mapping = OpenFileMappingW(
            FILE_MAP_WRITE, FALSE, dash_mapping_name);
        const HANDLE event = OpenEventW(
            EVENT_MODIFY_STATE, FALSE, dash_event_name);
        if (mapping != nullptr && event != nullptr) {
            auto* shared = static_cast<SharedTarget*>(MapViewOfFile(
                mapping, FILE_MAP_WRITE, 0, 0, sizeof(SharedTarget)));
            if (shared != nullptr) {
                const HWND existing = reinterpret_cast<HWND>(
                    shared->owner_console);
                shared->window = initial_target;
                if (existing != nullptr) {
                    (void)ac::console::activate_window(existing);
                }
                UnmapViewOfFile(shared);
            }
            SetEvent(event);
        }
        if (event != nullptr) CloseHandle(event);
        if (mapping != nullptr) CloseHandle(mapping);
        CloseHandle(instance_mutex);
        return false;
    }

    const HANDLE mapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
        sizeof(SharedTarget), dash_mapping_name);
    const HANDLE event = CreateEventW(
        nullptr, FALSE, FALSE, dash_event_name);
    if (mapping == nullptr || event == nullptr) {
        if (event != nullptr) CloseHandle(event);
        if (mapping != nullptr) CloseHandle(mapping);
        CloseHandle(instance_mutex);
        throw std::runtime_error("Unable to initialize Dash activation.");
    }
    auto* shared = static_cast<SharedTarget*>(MapViewOfFile(
        mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedTarget)));
    if (shared == nullptr) {
        CloseHandle(event);
        CloseHandle(mapping);
        CloseHandle(instance_mutex);
        throw std::runtime_error("Unable to share the Dash destination.");
    }
    shared->window = initial_target;
    shared->owner_console = reinterpret_cast<std::uintptr_t>(
        GetConsoleWindow());
    destination_window.store(initial_target);

    std::thread([instance_mutex, mapping, event, shared] {
        while (WaitForSingleObject(event, INFINITE) == WAIT_OBJECT_0) {
            destination_window.store(shared->window);
            (void)ac::console::activate();
        }
        UnmapViewOfFile(shared);
        CloseHandle(event);
        CloseHandle(mapping);
        CloseHandle(instance_mutex);
    }).detach();
    return true;
}

void monitor_parent(std::optional<std::uint64_t> parent_id) {
    if (!parent_id || *parent_id == 0 ||
        *parent_id > (std::numeric_limits<DWORD>::max)()) {
        return;
    }
    const HANDLE parent = OpenProcess(
        SYNCHRONIZE, FALSE, static_cast<DWORD>(*parent_id));
    if (parent == nullptr) {
        return;
    }
    std::thread([parent] {
        if (WaitForSingleObject(parent, INFINITE) == WAIT_OBJECT_0) {
            CloseHandle(parent);
            ExitProcess(0);
        }
        CloseHandle(parent);
    }).detach();
}

std::optional<std::wstring> select_secret(const std::filesystem::path& path) {
    const auto records = dash::vault::load_vault(path);
    if (records.empty()) {
        std::wcout << L"No secrets have been stored.\n";
        return std::nullopt;
    }

    std::wcout << L"\nStored secrets:\n";
    for (std::size_t index = 0; index < records.size(); ++index) {
        std::wcout << index + 1 << L". " << records[index].name << L'\n';
    }
    std::wcout << L"Select a secret: ";
    const auto selection = read_selection(records.size());
    if (!selection) {
        throw std::runtime_error("Invalid secret selection.");
    }

    if (!dash::hello::verify_user()) {
        std::wcout << L"Windows Hello verification was not completed.\n";
        return std::nullopt;
    }

    return dash::vault::unprotect(records[*selection].protected_value);
}

bool send_unicode_text(std::wstring_view text) {
    if (text.empty()) {
        return true;
    }
    if (text.size() > (std::numeric_limits<UINT>::max)() / 2) {
        return false;
    }

    std::vector<INPUT> inputs;
    inputs.reserve(text.size() * 2);
    for (const wchar_t character : text) {
        INPUT down{};
        down.type = INPUT_KEYBOARD;
        down.ki.wScan = static_cast<WORD>(character);
        down.ki.dwFlags = KEYEVENTF_UNICODE;
        INPUT up = down;
        up.ki.dwFlags |= KEYEVENTF_KEYUP;
        inputs.push_back(down);
        inputs.push_back(up);
    }

    const UINT count = static_cast<UINT>(inputs.size());
    const UINT inserted = SendInput(count, inputs.data(), sizeof(INPUT));
    SecureZeroMemory(inputs.data(), inputs.size() * sizeof(INPUT));
    return inserted == count;
}

void insert_secret() {
    const std::scoped_lock prompt_lock {secret_prompt_mutex};
    const HWND target_window = acquire_destination();
    if (target_window == nullptr) {
        std::wcerr << L"No destination window was selected.\n";
        return;
    }

    std::array<wchar_t, 512> destination_title{};
    if (GetWindowTextW(target_window, destination_title.data(),
                       static_cast<int>(destination_title.size())) > 0) {
        std::wcout << L"Destination: " << destination_title.data() << L'\n';
    } else {
        std::wcout << L"Destination: untitled window\n";
    }

    auto value = select_secret(dash::vault::vault_path());
    if (!value) {
        return;
    }

    const auto activated = ac::console::activate_window(target_window);
    if (!activated || GetForegroundWindow() != target_window) {
        dash::vault::wipe(*value);
        std::wcerr << L"The destination window changed; the secret was not inserted.\n";
        return;
    }

    const bool inserted = send_unicode_text(*value);
    dash::vault::wipe(*value);
    if (!inserted) {
        std::wcerr << L"Windows did not insert the complete secret value.\n";
    }
}

int export_keymap_commands() {
    const auto loaded = ac::main::launch_descriptor::read_embedded(
        GetModuleHandleW(nullptr),
        "dash_ac.exe"
    );
    if (!loaded) {
        std::cerr << "Dash keymap command export failed: launch descriptor is "
                     "missing\n";
        return 1;
    }
    if (!loaded->ok) {
        std::cerr << "Dash keymap command export failed: "
                  << loaded->error << '\n';
        return 1;
    }

    auto commands = loaded->descriptor.commands;
    std::ranges::sort(commands);
    for (const std::string& name : commands) {
        std::cout << name << '\n';
    }
    std::cout.flush();
    return std::cout ? 0 : 1;
}

} // namespace

int wmain(int argument_count, wchar_t* arguments[]) {
    ac::shell::set_process_app_user_model_id();
    if (argument_count == 2 &&
        std::wstring_view {arguments[1]} == L"--export-keymap-commands") {
        return export_keymap_commands();
    }
    try {
        dash::hello::initialize();

        SetConsoleTitleW(dash_window_title);
        ac::Component dash_component {
            "dash",
            ac::logging::config::LoggingScope {
                "dash",
                ac::logging::config::LoggingFallback::off
            }
        };
        {
            const auto ini_path = ac::paths::config_directory() / "dash.ini";
            if (!ac::ini::read(ini_path)) {
                std::error_code exists_error;
                const bool present =
                    std::filesystem::exists(ini_path, exists_error);
                dash_component.report_ini_unavailable(present && !exists_error);
            }
        }
        const auto target = argument_number(
            argument_count, arguments, L"--target");
        if (!initialize_single_instance(static_cast<std::uintptr_t>(
                target.value_or(0)))) {
            return 0;
        }
        monitor_parent(argument_number(
            argument_count, arguments, L"--parent-pid"));

        if (const auto activated = ac::console::activate(); !activated) {
            std::wcerr << L"Unable to activate the Dash console.\n";
        }

        while (true) {
            std::wcout << L"\ndash - Auto Core secret storage\n\n"
                       << L"1. Select and insert a secret\n"
                       << L"2. Exit\n\n"
                       << L"Select an option: ";

            const auto selection = read_selection(2);
            if (!selection) {
                std::wcerr << L"Invalid menu option.\n";
                continue;
            }
            try {
                switch (*selection) {
                case 0: insert_secret(); break;
                case 1: return 0;
                default: break;
                }
            } catch (const std::exception& error) {
                std::cerr << "dash: " << error.what() << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "dash failed: " << error.what() << '\n';
        return 1;
    }
}
