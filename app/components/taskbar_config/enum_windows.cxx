/**
 * Reference: parse a generated runtime INI and count matching top-level
 * windows with EnumWindows.
 *
 * Eligible windows are visible and titled. [window] fields are
 * case-insensitive AND; a blank field is ignored.
 *
 *   process_name     QueryFullProcessImageNameW filename
 *   executable_path  full image path
 *   class            GetClassNameW
 *   application_id   HWND AppUserModelID (SHGetPropertyStoreForWindow)
 */

#include "enum_windows.h"

#include <Ole2.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <wrl/client.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace taskbar_windows {
    std::string_view IniDocument::find(
        const std::string_view section,
        const std::string_view key
    ) const {
        const auto found = values.find(std::format("{}.{}", section, key));
        return found == values.end() ? std::string_view {} : found->second;
    }

    std::string_view trim(const std::string_view value) noexcept {
        const auto first = value.find_first_not_of(" \t\r");
        if (first == std::string_view::npos) return {};
        const auto last = value.find_last_not_of(" \t\r");
        return value.substr(first, last - first + 1);
    }

    std::wstring_view trim(const std::wstring_view value) noexcept {
        const auto first = value.find_first_not_of(L" \t\r");
        if (first == std::wstring_view::npos) return {};
        const auto last = value.find_last_not_of(L" \t\r");
        return value.substr(first, last - first + 1);
    }

    std::string ascii_lower(const std::string_view value) {
        std::string result {value};
        std::ranges::transform(result, result.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return result;
    }

    std::optional<std::wstring> from_utf8(const std::string_view value) {
        if (value.empty()) return std::wstring {};
        if (value.size() > static_cast<std::size_t>(
                (std::numeric_limits<int>::max)())) return std::nullopt;
        const int size = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), nullptr, 0
        );
        if (size <= 0) return std::nullopt;
        std::wstring result(static_cast<std::size_t>(size), L'\0');
        if (MultiByteToWideChar(
                CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                static_cast<int>(value.size()), result.data(), size) != size) {
            return std::nullopt;
        }
        return result;
    }

    std::optional<std::string> to_utf8(const std::wstring_view value) {
        if (value.empty()) return std::string {};
        if (value.size() > static_cast<std::size_t>(
                (std::numeric_limits<int>::max)())) return std::nullopt;
        const int size = WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr
        );
        if (size <= 0) return std::nullopt;
        std::string result(static_cast<std::size_t>(size), '\0');
        if (WideCharToMultiByte(
                CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                static_cast<int>(value.size()), result.data(), size,
                nullptr, nullptr) != size) return std::nullopt;
        return result;
    }

    std::optional<IniDocument> read_ini(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input) return std::nullopt;
        IniDocument document;
        std::string section;
        std::string line;
        while (std::getline(input, line)) {
            const std::string_view value = trim(line);
            if (value.empty() || value.starts_with('#') ||
                value.starts_with(';')) continue;
            if (value.starts_with('[') && value.ends_with(']')) {
                section = ascii_lower(trim(
                    value.substr(1, value.size() - 2)
                ));
                continue;
            }
            const auto equals = value.find('=');
            if (section.empty() || equals == std::string_view::npos) continue;
            const std::string key = ascii_lower(trim(value.substr(0, equals)));
            document.values.insert_or_assign(
                std::format("{}.{}", section, key),
                std::string {trim(value.substr(equals + 1))}
            );
        }
        return document;
    }

    std::vector<ApplicationConfiguration> load_applications(
        const std::filesystem::path& directory
    ) {
        std::vector<std::filesystem::path> files;
        std::error_code error;
        for (std::filesystem::directory_iterator iterator(directory, error);
             !error && iterator != std::filesystem::directory_iterator {};
             iterator.increment(error)) {
            if (iterator->is_regular_file(error) &&
                ascii_lower(iterator->path().extension().string()) == ".ini") {
                files.push_back(iterator->path());
            }
        }
        std::ranges::sort(files);

        std::vector<ApplicationConfiguration> result;
        for (const auto& file : files) {
            const auto document = read_ini(file);
            if (!document) continue;
            ApplicationConfiguration application {
                .path = file,
                .key = ascii_lower(document->find("application", "key")),
                .application_id = std::string {
                    document->find("taskbar", "application_id")
                }
            };
            application.matcher.process_name = from_utf8(document->find(
                "window", "process_name")).value_or(L"");
            application.matcher.executable_path = from_utf8(document->find(
                "window", "executable_path")).value_or(L"");
            application.matcher.window_class = from_utf8(document->find(
                "window", "class")).value_or(L"");
            application.matcher.window_application_id = from_utf8(
                document->find("window", "application_id")).value_or(L"");
            application.fallback_executable_path = from_utf8(document->find(
                "fallback", "executable_path")).value_or(L"");
            application.activate_command = std::string {
                document->find("commands", "activate")
            };
            if (!application.key.empty()) result.push_back(std::move(application));
        }
        return result;
    }

    bool equal_insensitive(
        const std::wstring_view left,
        const std::wstring_view right
    ) {
        if (left.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()) ||
            right.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
            return false;
        }
        return CompareStringOrdinal(
            left.data(), static_cast<int>(left.size()), right.data(),
            static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
    }

    bool contains_insensitive(
        const std::wstring_view value,
        const std::wstring_view part
    ) {
        if (part.empty()) return true;
        if (value.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()) ||
            part.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
            return false;
        }
        return FindStringOrdinal(
            FIND_FROMSTART, value.data(), static_cast<int>(value.size()),
            part.data(), static_cast<int>(part.size()), TRUE) >= 0;
    }

    bool matches(const WindowInfo& window, const WindowMatcher& matcher) {
        if (!matcher.configured()) return false;
        if (!matcher.window_class.empty() &&
            !equal_insensitive(window.window_class, matcher.window_class)) {
            return false;
        }
        if (!matcher.process_name.empty() &&
            !equal_insensitive(window.process_name, matcher.process_name)) {
            return false;
        }
        if (!matcher.executable_path.empty() &&
            !equal_insensitive(window.process_path, matcher.executable_path)) {
            return false;
        }
        if (!matcher.window_application_id.empty() &&
            !equal_insensitive(window.application_id,
                               matcher.window_application_id)) {
            return false;
        }
        return true;
    }

    unsigned match_count(
        const std::vector<WindowInfo>& windows,
        const WindowMatcher& matcher
    ) {
        return static_cast<unsigned>(std::ranges::count_if(
            windows, [&](const auto& window) { return matches(window, matcher); }
        ));
    }

    namespace {
        std::wstring window_text(const HWND window) {
            const int length = GetWindowTextLengthW(window);
            if (length <= 0) return {};
            std::wstring value(static_cast<std::size_t>(length) + 1, L'\0');
            const int copied = GetWindowTextW(
                window, value.data(), static_cast<int>(value.size()));
            if (copied <= 0) return {};
            value.resize(static_cast<std::size_t>(copied));
            return value;
        }

        std::wstring window_class_name(const HWND window) {
            std::wstring value(256, L'\0');
            const int copied = GetClassNameW(
                window, value.data(), static_cast<int>(value.size()));
            if (copied <= 0) return {};
            value.resize(static_cast<std::size_t>(copied));
            return value;
        }

        std::wstring process_path(const DWORD process_id) {
            const HANDLE process = OpenProcess(
                PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
            if (!process) return {};
            struct Guard {
                HANDLE handle;
                ~Guard() { CloseHandle(handle); }
            } guard {process};
            std::wstring value(32'768, L'\0');
            DWORD size = static_cast<DWORD>(value.size());
            if (!QueryFullProcessImageNameW(process, 0, value.data(), &size)) {
                return {};
            }
            value.resize(size);
            return value;
        }

        std::wstring window_application_id(const HWND window) {
            ComPtr<IPropertyStore> store;
            if (FAILED(SHGetPropertyStoreForWindow(
                    window, IID_PPV_ARGS(&store)))) return {};
            PROPVARIANT value;
            PropVariantInit(&value);
            if (FAILED(store->GetValue(PKEY_AppUserModel_ID, &value))) {
                PropVariantClear(&value);
                return {};
            }
            std::wstring result;
            if (value.vt == VT_LPWSTR && value.pwszVal) result = value.pwszVal;
            PropVariantClear(&value);
            return result;
        }

        struct WindowEnumerationContext {
            std::vector<WindowInfo> windows;
            std::unordered_map<DWORD, std::wstring> process_paths;
        };

        BOOL CALLBACK collect_window(const HWND window, const LPARAM parameter) {
            if (!IsWindowVisible(window)) return TRUE;
            std::wstring title = window_text(window);
            if (title.empty()) return TRUE;
            auto& context = *reinterpret_cast<WindowEnumerationContext*>(parameter);
            DWORD process_id {};
            GetWindowThreadProcessId(window, &process_id);
            auto [path, inserted] = context.process_paths.try_emplace(process_id);
            if (inserted) path->second = process_path(process_id);
            context.windows.push_back(WindowInfo {
                .handle = window,
                .process_id = process_id,
                .title = std::move(title),
                .window_class = window_class_name(window),
                .process_path = path->second,
                .process_name = path->second.empty() ? std::wstring {} :
                    std::filesystem::path {path->second}.filename().wstring(),
                .application_id = window_application_id(window)
            });
            return TRUE;
        }
    }

    std::optional<std::vector<WindowInfo>> enumerate_windows() {
        WindowEnumerationContext context;
        if (!EnumWindows(collect_window, reinterpret_cast<LPARAM>(&context))) {
            return std::nullopt;
        }
        return std::move(context.windows);
    }
}
