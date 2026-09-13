#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dpapi.h>
#include <knownfolders.h>
#include <shlobj.h>
#include <userconsentverifierinterop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Security.Credentials.UI.h>
#include <winrt/base.h>

#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

import auto_core.core.console;

#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Shell32.lib")

namespace {

constexpr std::uint32_t vault_magic = 0x48534144; // "DASH"
constexpr std::uint32_t vault_version = 1;
constexpr std::uint32_t maximum_entry_count = 1'000;
constexpr std::uint32_t maximum_name_characters = 1'024;
constexpr std::uint32_t maximum_blob_bytes = 1024 * 1024;
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

struct SecretRecord {
    std::wstring name;
    std::vector<std::byte> protected_value;
};

class LocalBuffer {
public:
    LocalBuffer(BYTE* data, DWORD size) noexcept : data_(data), size_(size) {}
    ~LocalBuffer() {
        if (data_ != nullptr) {
            SecureZeroMemory(data_, size_);
            LocalFree(data_);
        }
    }

    LocalBuffer(const LocalBuffer&) = delete;
    LocalBuffer& operator=(const LocalBuffer&) = delete;

    BYTE* data() const noexcept { return data_; }
    DWORD size() const noexcept { return size_; }

private:
    BYTE* data_{};
    DWORD size_{};
};

void wipe(std::wstring& value) noexcept {
    if (!value.empty()) {
        SecureZeroMemory(value.data(), value.size() * sizeof(wchar_t));
        value.clear();
    }
}

void wipe(std::vector<std::byte>& value) noexcept {
    if (!value.empty()) {
        SecureZeroMemory(value.data(), value.size());
        value.clear();
    }
}

std::filesystem::path vault_path() {
    PWSTR local_app_data = nullptr;
    const HRESULT result = SHGetKnownFolderPath(
        FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &local_app_data);
    if (FAILED(result)) {
        throw std::runtime_error("Unable to find Local AppData.");
    }

    const std::filesystem::path directory =
        std::filesystem::path(local_app_data) / L"Auto Core";
    CoTaskMemFree(local_app_data);
    std::filesystem::create_directories(directory);
    return directory / L"dash.vault";
}

template <typename T>
bool read_value(std::istream& input, T& value) {
    return static_cast<bool>(input.read(
        reinterpret_cast<char*>(&value), sizeof(value)));
}

template <typename T>
void write_value(std::ostream& output, const T& value) {
    output.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

std::vector<SecretRecord> load_vault(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return {};
    }

    std::ifstream input(path, std::ios::binary);
    std::uint32_t magic{}, version{}, count{};
    if (!read_value(input, magic) || !read_value(input, version) ||
        !read_value(input, count) || magic != vault_magic ||
        version != vault_version || count > maximum_entry_count) {
        throw std::runtime_error("The dash vault is invalid or unsupported.");
    }

    std::vector<SecretRecord> records;
    records.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        std::uint32_t name_size{}, blob_size{};
        if (!read_value(input, name_size) || name_size > maximum_name_characters) {
            throw std::runtime_error("The dash vault contains an invalid name.");
        }

        SecretRecord record;
        record.name.resize(name_size);
        if (!input.read(reinterpret_cast<char*>(record.name.data()),
                        static_cast<std::streamsize>(name_size) * sizeof(wchar_t)) ||
            !read_value(input, blob_size) || blob_size == 0 ||
            blob_size > maximum_blob_bytes) {
            throw std::runtime_error("The dash vault contains an invalid entry.");
        }

        record.protected_value.resize(blob_size);
        if (!input.read(reinterpret_cast<char*>(record.protected_value.data()),
                        blob_size)) {
            throw std::runtime_error("The dash vault is truncated.");
        }
        records.push_back(std::move(record));
    }

    if (input.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("The dash vault contains unexpected data.");
    }
    return records;
}

void save_vault(const std::filesystem::path& path,
                const std::vector<SecretRecord>& records) {
    const auto temporary = path.wstring() + L".new";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Unable to create the dash vault.");
    }

    write_value(output, vault_magic);
    write_value(output, vault_version);
    write_value(output, static_cast<std::uint32_t>(records.size()));
    for (const auto& record : records) {
        const auto name_size = static_cast<std::uint32_t>(record.name.size());
        const auto blob_size =
            static_cast<std::uint32_t>(record.protected_value.size());
        write_value(output, name_size);
        output.write(reinterpret_cast<const char*>(record.name.data()),
                     static_cast<std::streamsize>(name_size) * sizeof(wchar_t));
        write_value(output, blob_size);
        output.write(reinterpret_cast<const char*>(record.protected_value.data()),
                     blob_size);
    }
    output.flush();
    if (!output) {
        throw std::runtime_error("Unable to write the dash vault.");
    }
    output.close();

    if (!MoveFileExW(temporary.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temporary.c_str());
        throw std::runtime_error("Unable to replace the dash vault.");
    }
}

std::vector<std::byte> protect(std::wstring_view plaintext) {
    const auto byte_count = plaintext.size() * sizeof(wchar_t);
    if (byte_count == 0 ||
        byte_count > (std::numeric_limits<DWORD>::max)()) {
        throw std::runtime_error("The secret value is empty or too large.");
    }

    DATA_BLOB input{
        static_cast<DWORD>(byte_count),
        reinterpret_cast<BYTE*>(const_cast<wchar_t*>(plaintext.data()))};
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"Auto Core dash secret", nullptr, nullptr,
                          nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        throw std::runtime_error("Windows could not protect the secret value.");
    }

    LocalBuffer protected_data(output.pbData, output.cbData);
    return {reinterpret_cast<std::byte*>(protected_data.data()),
            reinterpret_cast<std::byte*>(protected_data.data()) +
                protected_data.size()};
}

std::wstring unprotect(std::span<const std::byte> ciphertext) {
    DATA_BLOB input{
        static_cast<DWORD>(ciphertext.size()),
        reinterpret_cast<BYTE*>(const_cast<std::byte*>(ciphertext.data()))};
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        throw std::runtime_error("Windows could not decrypt the secret value.");
    }

    LocalBuffer plaintext(output.pbData, output.cbData);
    if (plaintext.size() == 0 || plaintext.size() % sizeof(wchar_t) != 0) {
        throw std::runtime_error("The decrypted key value is invalid.");
    }
    return {reinterpret_cast<wchar_t*>(plaintext.data()),
            plaintext.size() / sizeof(wchar_t)};
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

bool is_windows_credential_dialog(HWND window) {
    if (!IsWindowVisible(window)) {
        return false;
    }

    std::array<wchar_t, 256> class_name{};
    std::array<wchar_t, 256> title{};
    if (GetClassNameW(window, class_name.data(),
                      static_cast<int>(class_name.size())) <= 0 ||
        GetWindowTextW(window, title.data(),
                       static_cast<int>(title.size())) <= 0 ||
        std::wstring_view {class_name.data()} !=
            L"Credential Dialog Xaml Host" ||
        std::wstring_view {title.data()} != L"Windows Security") {
        return false;
    }

    DWORD process_id{};
    GetWindowThreadProcessId(window, &process_id);
    const HANDLE process = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
    if (process == nullptr) {
        return false;
    }

    std::array<wchar_t, 32'768> image_path{};
    DWORD image_size = static_cast<DWORD>(image_path.size());
    const bool queried = QueryFullProcessImageNameW(
        process, 0, image_path.data(), &image_size) != FALSE;
    CloseHandle(process);
    if (!queried) {
        return false;
    }

    const std::filesystem::path image {
        std::wstring_view {image_path.data(), image_size}};
    if (_wcsicmp(image.filename().c_str(),
                 L"CredentialUIBroker.exe") != 0) {
        return false;
    }

    std::array<wchar_t, MAX_PATH> windows_directory{};
    const UINT windows_size = GetWindowsDirectoryW(
        windows_directory.data(),
        static_cast<UINT>(windows_directory.size()));
    if (windows_size == 0 || windows_size >= windows_directory.size()) {
        return false;
    }

    const std::wstring_view full_path {image_path.data(), image_size};
    const std::wstring_view windows_path {
        windows_directory.data(), windows_size};
    return full_path.size() > windows_path.size() &&
        _wcsnicmp(full_path.data(), windows_path.data(),
                  windows_path.size()) == 0 &&
        (full_path[windows_path.size()] == L'\\' ||
         full_path[windows_path.size()] == L'/');
}

HWND find_windows_credential_dialog() {
    HWND found = nullptr;
    EnumWindows(
        [](HWND window, LPARAM parameter) -> BOOL {
            auto& result = *reinterpret_cast<HWND*>(parameter);
            if (is_windows_credential_dialog(window)) {
                result = window;
                return FALSE;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&found));
    return found;
}

winrt::Windows::Security::Credentials::UI::UserConsentVerificationResult
wait_for_verification(
    const winrt::Windows::Foundation::IAsyncOperation<
        winrt::Windows::Security::Credentials::UI::
            UserConsentVerificationResult>& operation) {
    using winrt::Windows::Foundation::AsyncStatus;

    const HANDLE completed = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (completed == nullptr) {
        throw std::runtime_error(
            "Unable to create the Windows Hello completion event.");
    }

    try {
        operation.Completed(
            [completed](const auto&, AsyncStatus) noexcept {
                (void)SetEvent(completed);
            });

        bool credential_dialog_activated = false;
        const auto activation_deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds {5};
        while (true) {
            if (!credential_dialog_activated) {
                const HWND dialog = find_windows_credential_dialog();
                if (dialog != nullptr) {
                    const auto activated =
                        ac::console::activate_window(dialog);
                    credential_dialog_activated =
                        activated && GetForegroundWindow() == dialog;
                }
                if (!credential_dialog_activated &&
                    std::chrono::steady_clock::now() >=
                        activation_deadline) {
                    operation.Cancel();
                    throw std::runtime_error(
                        "Unable to activate the Windows Security dialog.");
                }
            }

            const DWORD wait_result = MsgWaitForMultipleObjects(
                1, &completed, FALSE, 25, QS_ALLINPUT);
            if (wait_result == WAIT_OBJECT_0) {
                break;
            }
            if (wait_result == WAIT_TIMEOUT) {
                continue;
            }
            if (wait_result != WAIT_OBJECT_0 + 1) {
                throw std::runtime_error(
                    "Windows Hello verification wait failed.");
            }

            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }

        const auto result = operation.GetResults();
        CloseHandle(completed);
        return result;
    } catch (...) {
        CloseHandle(completed);
        throw;
    }
}

bool verify_user() {
    using winrt::Windows::Foundation::IAsyncOperation;
    using winrt::Windows::Security::Credentials::UI::UserConsentVerificationResult;
    using winrt::Windows::Security::Credentials::UI::UserConsentVerifier;

    const HWND console_window = GetConsoleWindow();
    if (console_window == nullptr) {
        throw std::runtime_error(
            "Unable to attach Windows Hello to the Dash console.");
    }

    const auto interop =
        winrt::get_activation_factory<UserConsentVerifier,
                                      IUserConsentVerifierInterop>();
    IAsyncOperation<UserConsentVerificationResult> operation{nullptr};
    const winrt::hstring message =
        L"Verify your identity to retrieve this Auto Core key.";
    winrt::check_hresult(interop->RequestVerificationForWindowAsync(
        console_window,
        static_cast<HSTRING>(winrt::get_abi(message)),
        winrt::guid_of<IAsyncOperation<UserConsentVerificationResult>>(),
        winrt::put_abi(operation)));
    const bool verified = wait_for_verification(operation) ==
        UserConsentVerificationResult::Verified;
    (void)ac::console::activate();
    return verified;
}

void add_key(const std::filesystem::path& path) {
    auto records = load_vault(path);
    if (records.size() >= maximum_entry_count) {
        throw std::runtime_error("The dash vault has reached its entry limit.");
    }

    std::wcout << L"Secret name: ";
    std::wstring name;
    std::getline(std::wcin, name);
    if (name.empty() || name.size() > maximum_name_characters ||
        std::ranges::any_of(name, [](wchar_t character) {
            return std::iswcntrl(character) != 0;
        })) {
        throw std::runtime_error(
            "The secret name is empty, too long, or contains a control character.");
    }
    if (std::ranges::any_of(records, [&name](const SecretRecord& record) {
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
        records.push_back({std::move(name), protect(value)});
        wipe(value);
        save_vault(path, records);
    } catch (...) {
        wipe(value);
        throw;
    }
    std::wcout << L"Secret stored with current-user DPAPI.\n";
}

std::optional<std::wstring> select_key(const std::filesystem::path& path) {
    const auto records = load_vault(path);
    if (records.empty()) {
        std::wcout << L"No keys have been stored.\n";
        return std::nullopt;
    }

    std::wcout << L"\nStored keys:\n";
    for (std::size_t index = 0; index < records.size(); ++index) {
        std::wcout << index + 1 << L". " << records[index].name << L'\n';
    }
    std::wcout << L"Select a secret: ";
    const auto selection = read_selection(records.size());
    if (!selection) {
        throw std::runtime_error("Invalid key selection.");
    }

    if (!verify_user()) {
        std::wcout << L"Windows Hello verification was not completed.\n";
        return std::nullopt;
    }

    return unprotect(records[*selection].protected_value);
}

void list_secrets(const std::filesystem::path& path) {
    const auto records = load_vault(path);
    if (records.empty()) {
        std::wcout << L"No secrets have been stored.\n";
        return;
    }
    std::wcout << L"\nStored secrets:\n";
    for (std::size_t index = 0; index < records.size(); ++index) {
        std::wcout << index + 1 << L". " << records[index].name << L'\n';
    }
}

void remove_secret(const std::filesystem::path& path) {
    auto records = load_vault(path);
    if (records.empty()) {
        std::wcout << L"No secrets have been stored.\n";
        return;
    }
    std::wcout << L"\nStored secrets:\n";
    for (std::size_t index = 0; index < records.size(); ++index) {
        std::wcout << index + 1 << L". " << records[index].name << L'\n';
    }
    std::wcout << L"Select a secret to remove: ";
    const auto selection = read_selection(records.size());
    if (!selection) {
        throw std::runtime_error("Invalid secret selection.");
    }
    if (!verify_user()) {
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
    wipe(records[*selection].protected_value);
    records.erase(records.begin() +
        static_cast<std::ptrdiff_t>(*selection));
    save_vault(path, records);
    std::wcout << L"Secret removed.\n";
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

    auto value = select_key(vault_path());
    if (!value) {
        return;
    }

    const auto activated = ac::console::activate_window(target_window);
    if (!activated || GetForegroundWindow() != target_window) {
        wipe(*value);
        std::wcerr << L"The destination window changed; the secret was not inserted.\n";
        return;
    }

    const bool inserted = send_unicode_text(*value);
    wipe(*value);
    if (!inserted) {
        std::wcerr << L"Windows did not insert the complete secret value.\n";
    }
}

} // namespace

int wmain(int argument_count, wchar_t* arguments[]) {
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);

        if (argument_count == 3 &&
            std::wstring_view {arguments[1]} ==
                L"--generate-keymap-command-registry") {
            std::ofstream output(
                std::filesystem::path {arguments[2]},
                std::ios::binary | std::ios::trunc);
            output << "launch_dash\n";
            return output ? 0 : 1;
        }

        SetConsoleTitleW(dash_window_title);
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

        const auto path = vault_path();
        while (true) {
            std::wcout << L"\ndash - Auto Core secret storage\n\n"
                       << L"1. Add a secret\n"
                       << L"2. Select and insert a secret\n"
                       << L"3. Remove a secret\n"
                       << L"4. List secrets\n"
                       << L"5. Exit\n\n"
                       << L"Select an option: ";

            const auto selection = read_selection(5);
            if (!selection) {
                std::wcerr << L"Invalid menu option.\n";
                continue;
            }
            try {
                switch (*selection) {
                case 0: add_key(path); break;
                case 1: insert_secret(); break;
                case 2: remove_secret(path); break;
                case 3: list_secrets(path); break;
                case 4: return 0;
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
