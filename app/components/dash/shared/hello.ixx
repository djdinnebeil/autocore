/**
 * \file hello.ixx
 * \brief Windows Hello verification for Dash retrieval and removal.
 *
 * Add does not call this. The check is application-level user presence; it
 * is not bound to DPAPI.
 */
module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <userconsentverifierinterop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Security.Credentials.UI.h>
#include <winrt/base.h>

export module dash_hello;

import std;
import auto_core.core.console;

namespace {

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

} // namespace

export namespace dash::hello {

void initialize() {
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
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
        L"Verify your identity to retrieve this Auto Core secret.";
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

} // namespace dash::hello
