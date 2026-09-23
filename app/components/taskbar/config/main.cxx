#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Ole2.h>
#include <OleAuto.h>
#include <UIAutomation.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <wrl/client.h>

#include "enum_windows.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <io.h>
#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

import auto_core.core.component;
import auto_core.core.paths;
import taskbar_defaults;
import components_editor_request;

using Microsoft::WRL::ComPtr;
using taskbar_windows::ApplicationConfiguration;
using taskbar_windows::WindowInfo;
using taskbar_windows::WindowMatcher;
using taskbar_windows::ascii_lower;
using taskbar_windows::contains_insensitive;
using taskbar_windows::enumerate_windows;
using taskbar_windows::equal_insensitive;
using taskbar_windows::from_utf8;
using taskbar_windows::load_applications;
using taskbar_windows::matches;
using taskbar_windows::to_utf8;
using taskbar_windows::trim;

ac::Component taskbar_config {"taskbar_config"};

namespace {
    struct ToolConfiguration {
        std::wstring taskbar_window_class;
        std::wstring button_class;
        std::wstring application_id_prefix;
        std::wstring running_window_marker;
        std::filesystem::path applications_directory;
    };

    struct TaskbarButton {
        std::wstring name;
        std::string application_id;
        unsigned window_count {};
    };

    struct GeneratedConfiguration {
        TaskbarButton button;
        ApplicationConfiguration application;
        std::wstring reason;
        bool created {};
    };

    ToolConfiguration default_configuration() {
        return {
            .taskbar_window_class = L"Shell_TrayWnd",
            .button_class = L"Taskbar.TaskListButtonAutomationPeer",
            .application_id_prefix = L"Appid: ",
            .running_window_marker = L"running window",
            .applications_directory =
                ac::paths::taskbar_applications_directory()
        };
    }

    std::wstring take_bstr(BSTR value) {
        if (!value) return {};
        std::wstring result(value, SysStringLen(value));
        SysFreeString(value);
        return result;
    }

    std::optional<unsigned> parse_count(
        const std::wstring_view name,
        const std::wstring_view marker
    ) {
        const auto marker_position = name.find(marker);
        if (marker_position == std::wstring::npos) return 0;
        std::size_t end = marker_position;
        while (end && name[end - 1] == L' ') --end;
        std::size_t begin = end;
        while (begin && name[begin - 1] >= L'0' && name[begin - 1] <= L'9') --begin;
        if (begin == end) return std::nullopt;
        unsigned result {};
        for (std::size_t index = begin; index < end; ++index) {
            const unsigned digit = static_cast<unsigned>(name[index] - L'0');
            if (result > ((std::numeric_limits<unsigned>::max)() - digit) / 10) {
                return std::nullopt;
            }
            result = result * 10 + digit;
        }
        return result;
    }

    ComPtr<IUIAutomationCondition> property_condition(
        IUIAutomation* automation,
        const PROPERTYID property,
        const std::wstring_view setting
    ) {
        VARIANT value;
        VariantInit(&value);
        value.vt = VT_BSTR;
        value.bstrVal = SysAllocStringLen(setting.data(),
                                         static_cast<UINT>(setting.size()));
        if (!value.bstrVal) return {};
        ComPtr<IUIAutomationCondition> condition;
        const HRESULT result = automation->CreatePropertyCondition(
            property, value, &condition);
        VariantClear(&value);
        return SUCCEEDED(result) ? condition : nullptr;
    }

    std::optional<std::vector<TaskbarButton>> discover_taskbar_buttons(
        const ToolConfiguration& configuration
    ) {
        const HWND taskbar = FindWindowW(
            configuration.taskbar_window_class.c_str(), nullptr);
        if (!taskbar) return std::nullopt;
        ComPtr<IUIAutomation> automation;
        if (FAILED(CoCreateInstance(
                CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&automation)))) return std::nullopt;
        ComPtr<IUIAutomationElement> root;
        if (FAILED(automation->ElementFromHandle(taskbar, &root))) {
            return std::nullopt;
        }
        const auto condition = property_condition(
            automation.Get(), UIA_ClassNamePropertyId,
            configuration.button_class);
        ComPtr<IUIAutomationCacheRequest> cache;
        if (!condition || FAILED(automation->CreateCacheRequest(&cache)) ||
            FAILED(cache->AddProperty(UIA_NamePropertyId)) ||
            FAILED(cache->AddProperty(UIA_AutomationIdPropertyId)) ||
            FAILED(cache->put_TreeScope(TreeScope_Element)) ||
            FAILED(cache->put_AutomationElementMode(AutomationElementMode_None))) {
            return std::nullopt;
        }
        ComPtr<IUIAutomationElementArray> elements;
        if (FAILED(root->FindAllBuildCache(
                TreeScope_Descendants, condition.Get(), cache.Get(),
                &elements))) return std::nullopt;
        int length {};
        if (FAILED(elements->get_Length(&length))) return std::nullopt;
        std::vector<TaskbarButton> buttons;
        for (int index = 0; index < length; ++index) {
            ComPtr<IUIAutomationElement> element;
            BSTR raw_name {}, raw_id {};
            if (FAILED(elements->GetElement(index, &element)) ||
                FAILED(element->get_CachedName(&raw_name)) ||
                FAILED(element->get_CachedAutomationId(&raw_id))) continue;
            std::wstring name = take_bstr(raw_name);
            std::wstring id = take_bstr(raw_id);
            if (!id.starts_with(configuration.application_id_prefix)) continue;
            const auto count = parse_count(name, configuration.running_window_marker);
            std::wstring_view application_id = id;
            application_id.remove_prefix(
                configuration.application_id_prefix.size());
            const auto first = application_id.find_first_not_of(L" \t\r");
            application_id = first == std::wstring_view::npos
                ? std::wstring_view {}
                : application_id.substr(first);
            const auto utf8_id = to_utf8(application_id);
            if (count && utf8_id) buttons.push_back(TaskbarButton {
                .name = std::move(name),
                .application_id = std::move(*utf8_id),
                .window_count = *count
            });
        }
        return buttons;
    }

    std::string identity_token(const std::string_view value) {
        std::string result;
        for (const unsigned char ch : value) {
            if (std::isalnum(ch)) result.push_back(static_cast<char>(std::tolower(ch)));
        }
        return result;
    }

    const ApplicationConfiguration* find_configuration(
        const std::vector<ApplicationConfiguration>& applications,
        const TaskbarButton& button
    ) {
        const auto exact = std::ranges::find_if(applications, [&](const auto& item) {
            return !item.application_id.empty() &&
                ascii_lower(item.application_id) == ascii_lower(button.application_id);
        });
        if (exact != applications.end()) return &*exact;
        const std::string id = identity_token(button.application_id);
        const auto fallback = std::ranges::find_if(applications, [&](const auto& item) {
            const std::string key = identity_token(item.key);
            return item.application_id.empty() && !key.empty() && id.contains(key);
        });
        return fallback == applications.end() ? nullptr : &*fallback;
    }

    std::wstring display_base(std::wstring name) {
        constexpr std::wstring_view pinned = L" pinned";
        if (name.ends_with(pinned)) name.resize(name.size() - pinned.size());
        const auto marker = name.rfind(L" - ");
        if (marker != std::wstring::npos &&
            contains_insensitive(name.substr(marker + 3), L"running window")) {
            name.resize(marker);
        }
        return name;
    }

    std::string generated_key(const std::wstring_view display_name) {
        const auto utf8 = to_utf8(display_name).value_or("application");
        std::string result;
        bool separator = false;
        for (const unsigned char character : utf8) {
            if (std::isalnum(character)) {
                if (separator && !result.empty()) result.push_back('_');
                result.push_back(static_cast<char>(std::tolower(character)));
                separator = false;
            }
            else separator = true;
        }
        while (!result.empty() && result.back() == '_') result.pop_back();
        if (result.empty() || !std::isalpha(static_cast<unsigned char>(result.front()))) {
            result.insert(0, "application_");
        }
        return result;
    }

    constexpr std::string_view explorer_application_id =
        "Microsoft.Windows.Explorer";
    constexpr std::string_view firefox_application_id = "308046B0AF4A39CB";
    constexpr std::string_view firefox_private_application_id =
        "308046B0AF4A39CB;PrivateBrowsingAUMID";
    constexpr std::string_view acrobat_application_id = "AcrobatReader";
    constexpr std::wstring_view explorer_class = L"CabinetWClass";
    constexpr std::wstring_view explorer_fallback = L"C:\\Windows\\explorer.exe";

    bool is_explorer_id(const std::string_view id) {
        return ascii_lower(id) == ascii_lower(explorer_application_id);
    }

    bool is_firefox_id(const std::string_view id) {
        return ascii_lower(id) == ascii_lower(firefox_application_id);
    }

    bool is_firefox_private_id(const std::string_view id) {
        return ascii_lower(id) == ascii_lower(firefox_private_application_id);
    }

    bool is_firefox_family_id(const std::string_view id) {
        return is_firefox_id(id) || is_firefox_private_id(id);
    }

    bool is_acrobat_id(const std::string_view id) {
        return ascii_lower(id) == ascii_lower(acrobat_application_id);
    }

    bool identity_contains(
        const std::string_view id,
        const std::string_view key,
        const std::string_view token
    ) {
        return ascii_lower(id).contains(token) || ascii_lower(key).contains(token);
    }

    bool is_auto_core_identity(
        const std::string_view id,
        const std::string_view key = {}
    ) {
        return identity_contains(id, key, "auto_core");
    }

    bool is_itunes_identity(
        const std::string_view id,
        const std::string_view key = {}
    ) {
        return ascii_lower(id) == "apple.itunes" ||
            identity_contains(id, key, "itunes");
    }

    bool is_spotify_identity(
        const std::string_view id,
        const std::string_view key = {}
    ) {
        return identity_contains(id, key, "spotify");
    }

    bool is_discord_identity(
        const std::string_view id,
        const std::string_view key = {}
    ) {
        return ascii_lower(id) == "com.squirrel.discord.discord" ||
            identity_contains(id, key, "discord");
    }

    bool is_zoom_identity(
        const std::string_view id,
        const std::string_view key = {}
    ) {
        return ascii_lower(id).starts_with("zoom.us.") ||
            identity_contains(id, key, "zoom");
    }

    bool is_teams_identity(
        const std::string_view id,
        const std::string_view key = {}
    ) {
        return ascii_lower(id).starts_with("msteams_") ||
            identity_contains(id, key, "microsoft_teams") ||
            identity_contains(id, key, "msteams");
    }

    bool catalog_one_shot(
        const std::string_view application_id,
        const std::string_view key = {}
    ) {
        return is_auto_core_identity(application_id, key) ||
            is_itunes_identity(application_id, key) ||
            is_spotify_identity(application_id, key) ||
            is_discord_identity(application_id, key) ||
            is_zoom_identity(application_id, key) ||
            is_teams_identity(application_id, key);
    }

    WindowMatcher catalog_matcher(
        const std::string_view application_id,
        const std::string_view key = {}
    ) {
        WindowMatcher matcher;
        if (is_explorer_id(application_id)) {
            matcher.process_name = L"explorer.exe";
            matcher.window_class = std::wstring {explorer_class};
        }
        else if (is_firefox_family_id(application_id)) {
            matcher.process_name = L"firefox.exe";
            matcher.window_application_id =
                from_utf8(application_id).value_or(L"");
        }
        else if (is_acrobat_id(application_id)) {
            matcher.process_name = L"Acrobat.exe";
        }
        else if (is_auto_core_identity(application_id, key)) {
            matcher.process_name = L"auto_core.exe";
        }
        else if (is_itunes_identity(application_id, key)) {
            matcher.process_name = L"iTunes.exe";
        }
        else if (is_spotify_identity(application_id, key)) {
            matcher.process_name = L"Spotify.exe";
        }
        else if (is_discord_identity(application_id, key)) {
            matcher.process_name = L"discord.exe";
        }
        else if (is_zoom_identity(application_id, key)) {
            matcher.process_name = L"Zoom.exe";
        }
        else if (is_teams_identity(application_id, key)) {
            matcher.process_name = L"ms-teams.exe";
        }
        return matcher;
    }

    bool is_chrome_id(const std::string_view id) {
        return ascii_lower(id) == "chrome";
    }

    bool is_visual_studio_code_id(const std::string_view id) {
        return ascii_lower(id) == "microsoft.visualstudiocode";
    }

    bool is_visual_studio_id(const std::string_view id) {
        const std::string lower = ascii_lower(id);
        return lower.starts_with("visualstudio.") &&
            !lower.starts_with("visualstudiocode");
    }

    std::string catalog_key(
        const std::string_view application_id,
        const std::string_view key = {}
    ) {
        if (is_explorer_id(application_id)) return "file_explorer";
        if (is_firefox_id(application_id)) return "firefox";
        if (is_firefox_private_id(application_id)) return "firefox_private_browsing";
        if (is_chrome_id(application_id)) return "google_chrome";
        if (is_visual_studio_code_id(application_id)) return "visual_studio_code";
        if (is_visual_studio_id(application_id)) return "visual_studio";
        if (is_zoom_identity(application_id, key)) return "zoom_workplace";
        if (is_acrobat_id(application_id)) return "adobe_acrobat";
        if (is_auto_core_identity(application_id, key)) return "auto_core";
        if (is_itunes_identity(application_id, key)) return "itunes";
        if (is_spotify_identity(application_id, key)) return "spotify";
        if (is_discord_identity(application_id, key)) return "discord";
        if (is_teams_identity(application_id, key)) return "microsoft_teams";
        return {};
    }

    std::string activate_commands_for_key(const std::string& key) {
        std::string activate = "activate_" + key;
        if (key == "file_explorer") return activate + " | activate_folder";
        if (key == "google_chrome") return activate + " | activate_chrome";
        if (key == "visual_studio") return activate + " | activate_visual";
        if (key == "visual_studio_code") {
            return activate + " | activate_vs_code";
        }
        if (key == "zoom_workplace") return activate + " | activate_zoom";
        return activate;
    }

    bool is_packaged_application_id(const std::string_view application_id) {
        return application_id.contains('!');
    }

    std::wstring packaged_app_fallback(const std::string_view application_id) {
        const auto aumid = from_utf8(application_id);
        if (!aumid || aumid->empty()) return {};
        return L"shell:AppsFolder\\" + *aumid;
    }

    std::wstring catalog_fallback(const std::string_view application_id) {
        if (is_explorer_id(application_id) &&
            std::filesystem::exists(std::filesystem::path {explorer_fallback})) {
            return std::wstring {explorer_fallback};
        }
        if (is_firefox_family_id(application_id)) {
            const std::filesystem::path paths[] {
                L"C:\\Program Files\\Mozilla Firefox\\firefox.exe",
                L"C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe"
            };
            for (const auto& path : paths) {
                if (std::filesystem::exists(path)) return path.wstring();
            }
        }
        if (is_acrobat_id(application_id)) {
            return std::wstring {L"[]"};
        }
        if (is_packaged_application_id(application_id)) {
            return packaged_app_fallback(application_id);
        }
        return {};
    }

    bool usable_process_name(const std::wstring_view name) {
        return !name.empty() &&
            !equal_insensitive(name, L"Update.exe") &&
            !equal_insensitive(name, L"ApplicationFrameHost.exe");
    }

    std::wstring exe_filename(const std::wstring_view path) {
        if (path.size() < 4 ||
            !equal_insensitive(path.substr(path.size() - 4), L".exe")) {
            return {};
        }
        const auto separator = path.find_last_of(L"\\/");
        const auto name = separator == std::wstring_view::npos
            ? path : path.substr(separator + 1);
        return usable_process_name(name) ? std::wstring {name} : std::wstring {};
    }

    std::wstring process_name_from_application_id(const std::string_view id) {
        const auto wide = from_utf8(id).value_or(L"");
        if (wide.empty()) return {};
        if (const auto from_path = exe_filename(wide); !from_path.empty()) {
            return from_path;
        }
        constexpr std::wstring_view marker = L".exe.";
        if (wide.size() > static_cast<std::size_t>(
                (std::numeric_limits<int>::max)()) ||
            marker.size() > static_cast<std::size_t>(
                (std::numeric_limits<int>::max)())) {
            return {};
        }
        const int found = FindStringOrdinal(
            FIND_FROMSTART, wide.data(), static_cast<int>(wide.size()),
            marker.data(), static_cast<int>(marker.size()), TRUE);
        if (found <= 0) return {};
        const std::size_t end = static_cast<std::size_t>(found);
        std::size_t begin = end;
        while (begin > 0 && wide[begin - 1] != L'.') --begin;
        if (begin == end) return {};
        std::wstring name {wide.substr(begin, end - begin)};
        name += L".exe";
        return usable_process_name(name) ? name : std::wstring {};
    }

    std::filesystem::path user_pinned_taskbar_directory() {
        PWSTR pinned {};
        if (FAILED(SHGetKnownFolderPath(
                FOLDERID_UserPinned, 0, nullptr, &pinned))) {
            return {};
        }
        std::filesystem::path path {pinned};
        CoTaskMemFree(pinned);
        return path / L"TaskBar";
    }

    std::wstring shell_link_target(const std::filesystem::path& shortcut) {
        ComPtr<IShellLinkW> link;
        if (FAILED(CoCreateInstance(
                CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&link)))) return {};
        ComPtr<IPersistFile> persist;
        if (FAILED(link.As(&persist)) ||
            FAILED(persist->Load(shortcut.c_str(), STGM_READ))) return {};
        std::wstring value(32'768, L'\0');
        if (FAILED(link->GetPath(
                value.data(), static_cast<int>(value.size()), nullptr, 0))) {
            return {};
        }
        value.resize(std::wcslen(value.c_str()));
        return value;
    }

    std::wstring shell_link_application_id(const std::filesystem::path& shortcut) {
        ComPtr<IPropertyStore> store;
        if (FAILED(SHGetPropertyStoreFromParsingName(
                shortcut.c_str(), nullptr, GPS_DEFAULT,
                IID_PPV_ARGS(&store)))) return {};
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

    std::wstring pinned_shortcut_target(const TaskbarButton& button) {
        const auto directory = user_pinned_taskbar_directory();
        std::error_code error;
        if (directory.empty() || !std::filesystem::exists(directory, error)) {
            return {};
        }
        const std::wstring base = display_base(button.name);
        const auto button_id = from_utf8(button.application_id).value_or(L"");
        std::wstring by_name;
        for (std::filesystem::directory_iterator iterator(directory, error);
             !error && iterator != std::filesystem::directory_iterator {};
             iterator.increment(error)) {
            if (!iterator->is_regular_file(error) ||
                !equal_insensitive(iterator->path().extension().wstring(), L".lnk")) {
                continue;
            }
            const auto target = shell_link_target(iterator->path());
            if (target.empty()) continue;
            const auto aumid = shell_link_application_id(iterator->path());
            if (!button_id.empty() && !aumid.empty() &&
                equal_insensitive(aumid, button_id)) {
                return target;
            }
            if (!base.empty() &&
                equal_insensitive(iterator->path().stem().wstring(), base)) {
                by_name = target;
            }
        }
        return by_name;
    }

    const TaskbarButton* owning_button(
        const WindowInfo& window,
        const std::vector<TaskbarButton>& buttons
    ) {
        if (!window.application_id.empty()) {
            const auto exact = std::ranges::find_if(buttons, [&](const auto& button) {
                const auto id = from_utf8(button.application_id).value_or(L"");
                return !id.empty() &&
                    equal_insensitive(window.application_id, id);
            });
            if (exact != buttons.end()) return &*exact;
        }
        const TaskbarButton* best {};
        std::size_t best_size {};
        unsigned ties {};
        for (const auto& button : buttons) {
            const std::wstring base = display_base(button.name);
            if (base.empty() || !contains_insensitive(window.title, base)) continue;
            if (base.size() > best_size) {
                best = &button;
                best_size = base.size();
                ties = 1;
            }
            else if (base.size() == best_size) ++ties;
        }
        return ties == 1 ? best : nullptr;
    }

    std::vector<const WindowInfo*> candidate_windows(
        const TaskbarButton& button,
        const std::vector<TaskbarButton>& buttons,
        const std::vector<WindowInfo>& windows
    ) {
        std::vector<const WindowInfo*> result;
        for (const auto& window : windows) {
            const auto* owner = owning_button(window, buttons);
            if (owner && ascii_lower(owner->application_id) ==
                ascii_lower(button.application_id)) {
                result.push_back(&window);
            }
        }
        return result;
    }

    std::wstring extract_process_name(
        const TaskbarButton& button,
        const std::vector<TaskbarButton>& buttons,
        const std::vector<WindowInfo>& windows,
        const std::wstring_view fallback
    ) {
        if (const auto from_id = process_name_from_application_id(
                button.application_id); !from_id.empty()) {
            return from_id;
        }
        std::wstring from_window;
        for (const auto* window : candidate_windows(button, buttons, windows)) {
            if (!usable_process_name(window->process_name)) continue;
            if (from_window.empty()) from_window = window->process_name;
            else if (!equal_insensitive(from_window, window->process_name)) {
                from_window.clear();
                break;
            }
        }
        if (!from_window.empty()) return from_window;
        return exe_filename(fallback);
    }

    std::wstring prompt_process_name(const TaskbarButton& button) {
        const std::wstring id = from_utf8(button.application_id).value_or(L"");
        std::wcout << L"\nCould not derive process_name for "
                   << display_base(button.name) << L".\n"
                   << L"Taskbar application_id: "
                   << (id.empty() ? L"<unknown>" : id) << L'\n';
        if (!_isatty(_fileno(stdin))) {
            std::wcout << L"No console input; leaving process_name blank.\n";
            return {};
        }
        std::wcout << L"Enter the .exe name (e.g. chrome.exe), or press Enter "
                      L"to leave blank: ";
        std::wcout.flush();
        std::wstring line;
        std::getline(std::wcin, line);
        const auto entered = trim(std::wstring_view {line});
        if (entered.empty()) return {};
        if (const auto from_path = exe_filename(entered); !from_path.empty()) {
            return from_path;
        }
        return std::wstring {entered};
    }

    std::wstring prompt_fallback_path(const TaskbarButton& button) {
        const std::wstring id = from_utf8(button.application_id).value_or(L"");
        std::wcout << L"\nCould not derive fallback executable_path for "
                   << display_base(button.name) << L".\n"
                   << L"Taskbar application_id: "
                   << (id.empty() ? L"<unknown>" : id) << L'\n';
        if (!_isatty(_fileno(stdin))) {
            std::wcout << L"No console input; leaving fallback blank.\n";
            return {};
        }
        std::wcout << L"Enter the full .exe path, [] to disable launch, or "
                      L"press Enter to leave blank: ";
        std::wcout.flush();
        std::wstring line;
        std::getline(std::wcin, line);
        auto entered = trim(std::wstring_view {line});
        if (entered.size() >= 2 &&
            ((entered.front() == L'"' && entered.back() == L'"') ||
             (entered.front() == L'\'' && entered.back() == L'\''))) {
            entered.remove_prefix(1);
            entered.remove_suffix(1);
            entered = trim(entered);
        }
        return std::wstring {entered};
    }

    std::wstring prompt_window_executable_path(const TaskbarButton& button) {
        const std::wstring id = from_utf8(button.application_id).value_or(L"");
        std::wcout << L"\nprocess_name notepad.exe is shared by more than one "
                      L"program ("
                   << display_base(button.name) << L").\n"
                   << L"Taskbar application_id: "
                   << (id.empty() ? L"<unknown>" : id) << L'\n';
        if (!_isatty(_fileno(stdin))) {
            std::wcout << L"No console input; leaving window executable_path "
                          L"blank.\n";
            return {};
        }
        std::wcout << L"Enter [window] executable_path to distinguish this "
                      L"program, or press Enter to leave blank: ";
        std::wcout.flush();
        std::wstring line;
        std::getline(std::wcin, line);
        auto entered = trim(std::wstring_view {line});
        if (entered.size() >= 2 &&
            ((entered.front() == L'"' && entered.back() == L'"') ||
             (entered.front() == L'\'' && entered.back() == L'\''))) {
            entered.remove_prefix(1);
            entered.remove_suffix(1);
            entered = trim(entered);
        }
        return std::wstring {entered};
    }

    std::wstring owned_window_path(
        const TaskbarButton& button,
        const std::vector<TaskbarButton>& buttons,
        const std::vector<WindowInfo>& windows
    ) {
        std::wstring result;
        for (const auto* window : candidate_windows(button, buttons, windows)) {
            if (window->process_path.empty()) continue;
            if (result.empty()) result = window->process_path;
            else if (!equal_insensitive(result, window->process_path)) return {};
        }
        return result;
    }

    std::wstring stable_fallback_path(
        const std::vector<WindowInfo>& windows,
        const WindowMatcher& matcher
    ) {
        std::wstring result;
        for (const auto& window : windows) {
            if (!matches(window, matcher) || window.process_path.empty()) continue;
            if (result.empty()) result = window.process_path;
            else if (!equal_insensitive(result, window.process_path)) return {};
        }
        return result;
    }

    std::wstring infer_fallback(
        const TaskbarButton& button,
        const std::vector<TaskbarButton>& buttons,
        const std::vector<WindowInfo>& windows,
        const WindowMatcher& matcher
    ) {
        if (const auto catalog = catalog_fallback(button.application_id);
            !catalog.empty()) {
            return catalog;
        }
        if (const auto matched = stable_fallback_path(windows, matcher);
            !matched.empty()) {
            return matched;
        }
        if (const auto owned = owned_window_path(button, buttons, windows);
            !owned.empty()) {
            return owned;
        }
        return pinned_shortcut_target(button);
    }

    WindowMatcher generate_matcher(
        const TaskbarButton& button,
        const std::vector<TaskbarButton>& buttons,
        const std::vector<WindowInfo>& windows,
        const std::wstring_view fallback,
        std::wstring& reason
    ) {
        const std::string display_key = generated_key(display_base(button.name));
        WindowMatcher matcher = catalog_matcher(
            button.application_id, display_key
        );
        if (matcher.configured()) {
            reason = L"catalog";
            return matcher;
        }
        matcher.process_name = extract_process_name(
            button, buttons, windows, fallback);
        if (matcher.process_name.empty()) {
            matcher.process_name = prompt_process_name(button);
            reason = matcher.process_name.empty()
                ? L"process_name left blank" : L"process_name provided by the user";
        }
        else reason = L"derived process_name from the taskbar icon";
        if (equal_insensitive(matcher.process_name, L"notepad.exe")) {
            matcher.executable_path = prompt_window_executable_path(button);
            if (!reason.empty()) reason += L"; ";
            reason += matcher.executable_path.empty()
                ? L"notepad window executable_path left blank"
                : L"notepad window executable_path provided by the user";
        }
        return matcher;
    }

    std::wstring describe_matcher(const WindowMatcher& matcher) {
        std::wstring result;
        const auto add = [&](const std::wstring_view label, const std::wstring_view value) {
            if (value.empty()) return;
            if (!result.empty()) result += L"; ";
            result += std::format(L"{}={}", label, value);
        };
        add(L"process_name", matcher.process_name);
        add(L"executable_path", matcher.executable_path);
        add(L"class", matcher.window_class);
        add(L"application_id", matcher.window_application_id);
        return result.empty() ? L"(no matcher fields)" : result;
    }

    bool write_runtime_ini(const GeneratedConfiguration& item) {
        std::ofstream output(item.application.path, std::ios::trunc);
        if (!output) return false;
        const auto field = [&](const std::wstring_view value) {
            return to_utf8(value).value_or("");
        };
        const std::string activate = item.application.activate_command.empty()
            ? activate_commands_for_key(item.application.key)
            : item.application.activate_command;
        output << "[application]\nkey = " << item.application.key << "\n\n"
               << "[taskbar]\napplication_id = "
               << item.application.application_id << "\n\n"
               << "[window]\nprocess_name = "
               << field(item.application.matcher.process_name) << "\n"
               << "executable_path = "
               << field(item.application.matcher.executable_path) << "\n"
               << "class = " << field(item.application.matcher.window_class) << "\n"
               << "application_id = "
               << field(item.application.matcher.window_application_id)
               << "\n\n"
               << "[activation]\nmulti_window = "
               << (catalog_one_shot(
                       item.application.application_id, item.application.key
                   ) ? "one-shot" : "cycle")
               << "\n\n"
               << "[fallback]\nexecutable_path = "
               << field(item.application.fallback_executable_path) << "\n\n"
               << "[commands]\nactivate = " << activate << "\n";
        return static_cast<bool>(output);
    }

    GeneratedConfiguration resolve_program(
        const TaskbarButton& button,
        const std::vector<TaskbarButton>& buttons,
        const std::vector<WindowInfo>& discovery_windows,
        const std::vector<ApplicationConfiguration>& existing,
        std::set<std::string>& used_keys
    ) {
        GeneratedConfiguration item;
        item.button = button;
        const auto* found = find_configuration(existing, button);
        if (found) {
            item.application = *found;
            used_keys.insert(item.application.key);
            item.reason = L"already exists";
            return item;
        }
        const std::string display_key = generated_key(display_base(button.name));
        std::string key = catalog_key(button.application_id, display_key);
        if (key.empty()) key = display_key;
        const std::string original = key;
        unsigned suffix = 2;
        while (!used_keys.insert(key).second) {
            key = std::format("{}_{}", original, suffix++);
        }
        item.application.key = key;
        item.application.activate_command = activate_commands_for_key(key);
        item.application.application_id = button.application_id;
        item.application.fallback_executable_path = infer_fallback(
            button, buttons, discovery_windows, WindowMatcher {});
        item.application.matcher = generate_matcher(
            button, buttons, discovery_windows,
            item.application.fallback_executable_path, item.reason);
        if (const auto path = infer_fallback(
                button, buttons, discovery_windows,
                item.application.matcher); !path.empty()) {
            item.application.fallback_executable_path = path;
        }
        if (item.application.fallback_executable_path.empty()) {
            item.application.fallback_executable_path =
                prompt_fallback_path(button);
            if (!item.reason.empty()) item.reason += L"; ";
            item.reason +=
                item.application.fallback_executable_path.empty()
                    ? L"fallback left blank"
                    : L"fallback provided by the user";
        }
        return item;
    }

    bool write_missing_runtime_ini(
        const ToolConfiguration& configuration,
        GeneratedConfiguration& item
    ) {
        if (item.application.path.empty()) {
            item.application.path = configuration.applications_directory /
                from_utf8(item.application.key + ".ini").value_or(L"application.ini");
        }
        std::error_code error;
        if (std::filesystem::exists(item.application.path, error)) return true;
        if (!write_runtime_ini(item)) return false;
        item.created = true;
        return true;
    }

    bool ensure_firefox_pair(
        const ToolConfiguration& configuration,
        const std::vector<TaskbarButton>& buttons,
        std::set<std::string>& used_keys,
        std::vector<GeneratedConfiguration>& generated
    ) {
        const bool present = std::ranges::any_of(buttons, [&](const auto& button) {
            return is_firefox_family_id(button.application_id);
        });
        if (!present) return true;

        const auto write_variant = [&](const std::string_view key,
                                       const std::string_view application_id) {
            used_keys.insert(std::string {key});
            const auto path = configuration.applications_directory /
                from_utf8(std::string {key} + ".ini").value_or(L"application.ini");
            std::error_code error;
            if (std::filesystem::exists(path, error)) return true;
            GeneratedConfiguration item;
            item.application.key = std::string {key};
            item.application.application_id = std::string {application_id};
            item.application.matcher = catalog_matcher(application_id);
            item.application.fallback_executable_path =
                catalog_fallback(application_id);
            item.application.activate_command =
                activate_commands_for_key(item.application.key);
            item.application.path = path;
            item.reason = L"catalog";
            if (!write_runtime_ini(item)) return false;
            item.created = true;
            generated.push_back(std::move(item));
            return true;
        };
        return write_variant("firefox", firefox_application_id) &&
            write_variant("firefox_private_browsing", firefox_private_application_id);
    }

    bool generate_configurations(
        const ToolConfiguration& configuration,
        const std::vector<TaskbarButton>& buttons,
        const std::vector<WindowInfo>& discovery_windows,
        std::vector<GeneratedConfiguration>& generated
    ) {
        std::error_code error;
        std::filesystem::create_directories(
            configuration.applications_directory, error);
        if (error) return false;
        const auto existing = load_applications(
            configuration.applications_directory);
        std::set<std::string> used_keys;
        for (const auto& application : existing) used_keys.insert(application.key);
        for (const auto& button : buttons) {
            GeneratedConfiguration item = resolve_program(
                button, buttons, discovery_windows, existing, used_keys);
            if (!write_missing_runtime_ini(configuration, item)) return false;
            generated.push_back(std::move(item));
        }
        return ensure_firefox_pair(configuration, buttons, used_keys, generated);
    }

    void add_activate_names(
        std::set<std::string>& commands,
        const std::string_view setting
    ) {
        std::size_t first {};
        while (first <= setting.size()) {
            const auto separator = setting.find('|', first);
            std::string alias {setting.substr(
                first,
                separator == std::string_view::npos
                    ? std::string_view::npos
                    : separator - first
            )};
            const auto alias_begin = alias.find_first_not_of(" \t\r");
            if (alias_begin != std::string::npos) {
                const auto alias_end = alias.find_last_not_of(" \t\r");
                commands.insert(alias.substr(
                    alias_begin, alias_end - alias_begin + 1
                ));
            }
            if (separator == std::string_view::npos) break;
            first = separator + 1;
        }
    }

    bool write_keymap_manifest(const ToolConfiguration& configuration) {
        std::set<std::string> commands {
            "activate_auto_core",
            "activate_powershell_in_admin",
            "activate_wordpad",
            "launch_gitbash",
            "launch_powershell",
            "refresh_taskbar_positions"
        };
        for (const auto& application :
             load_applications(configuration.applications_directory)) {
            add_activate_names(commands, activate_commands_for_key(
                application.key
            ));
            add_activate_names(commands, application.activate_command);
        }

        const auto destination =
            ac::paths::keymap_components_directory() /
            L"taskbar.keymap_commands.txt";
        std::error_code error;
        std::filesystem::create_directories(destination.parent_path(), error);
        if (error) return false;

        std::filesystem::path temporary = destination;
        temporary += L".tmp";
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) return false;
        for (const std::string& command : commands) {
            output << command << '\n';
        }
        output.close();
        if (!output) {
            std::filesystem::remove(temporary, error);
            return false;
        }
        if (!MoveFileExW(
                temporary.c_str(),
                destination.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
            )) {
            std::filesystem::remove(temporary, error);
            return false;
        }
        std::wcout << L"Updated " << destination.wstring() << L'\n';
        return true;
    }

    int generate(const ToolConfiguration& configuration) {
        const auto buttons = discover_taskbar_buttons(configuration);
        const auto windows = enumerate_windows();
        if (!buttons || !windows) {
            std::wcerr << L"Unable to collect taskbar or window metadata.\n";
            taskbar_config.log_and_print(
                "Unable to collect taskbar or window metadata."
            );
            return 1;
        }

        std::vector<GeneratedConfiguration> generated;
        if (!generate_configurations(
                configuration, *buttons, *windows, generated)) {
            std::wcerr << L"Unable to write generated configurations.\n";
            taskbar_config.log_and_print(
                "Unable to write generated configurations."
            );
            return 1;
        }

        unsigned created {};
        unsigned existing {};
        std::wcout << L"Taskbar configuration generator\n"
                   << L"Output: " << configuration.applications_directory.wstring()
                   << L"\nTaskbar icons: " << buttons->size() << L"\n\n";
        for (const auto& item : generated) {
            const auto name = item.application.path.filename().wstring();
            if (item.created) {
                ++created;
                std::wcout << L"  created  " << name << L"  "
                           << describe_matcher(item.application.matcher)
                           << L"  (" << item.reason << L")\n";
            }
            else {
                ++existing;
                std::wcout << L"  exists   " << name << L'\n';
            }
        }
        std::wcout << L"\nCreated " << created
                   << L", already present " << existing << L".\n";
        if (!write_keymap_manifest(configuration)) {
            std::wcerr << L"Unable to update keymap/components/taskbar.keymap_commands.txt.\n";
            taskbar_config.log_and_print(
                "Unable to update keymap/components/taskbar.keymap_commands.txt."
            );
            return 1;
        }
        taskbar_config.log_and_log("taskbar configuration generated");
        return 0;
    }

    bool ensure_taskbar_ini(const bool prompt) {
        const auto path = ac::paths::config_directory() / "taskbar.ini";

        std::error_code error;
        if (std::filesystem::exists(path, error)) {
            return true;
        }
        if (error) {
            taskbar_config.log_and_print(
                "Failed to inspect {}: {}",
                path.string(),
                error.message()
            );
            return false;
        }

        std::wstring directory;
        if (prompt) {
            std::wcout << L"Taskbar application data directory [.\\taskbar]: ";
            std::wstring input;
            std::getline(std::wcin, input);
            directory = std::wstring {trim(std::wstring_view {input})};
        }

        const auto utf8 = to_utf8(directory);
        if (!utf8) {
            taskbar_config.log_and_print(
                "Failed to encode the taskbar data directory."
            );
            return false;
        }
        std::error_code create_error;
        std::filesystem::create_directories(
            ac::paths::config_directory(),
            create_error
        );
        if (create_error) {
            taskbar_config.log_and_print(
                "Failed to create config directory: {}",
                create_error.message()
            );
            return false;
        }
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            taskbar_config.log_and_print("Failed to create {}", path.string());
            return false;
        }
        const auto contents = taskbar::defaults::ini_for(
            utf8->empty() ? taskbar::defaults::directory : *utf8
        );
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.close();
        return static_cast<bool>(output);
    }

    int run() {
        const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(initialized)) return 1;
        struct ComGuard { ~ComGuard() { CoUninitialize(); } } guard;
        const HWND console = GetConsoleWindow();
        if (console != nullptr) {
            const HWND foreground = GetForegroundWindow();
            const DWORD current_thread = GetCurrentThreadId();
            const DWORD foreground_thread = foreground == nullptr
                ? 0
                : GetWindowThreadProcessId(foreground, nullptr);
            const bool attached = foreground_thread != 0 &&
                foreground_thread != current_thread &&
                AttachThreadInput(current_thread, foreground_thread, TRUE);
            if (IsIconic(console)) {
                (void)ShowWindow(console, SW_RESTORE);
            }
            (void)BringWindowToTop(console);
            (void)SetForegroundWindow(console);
            (void)SetFocus(console);
            if (attached) {
                (void)AttachThreadInput(
                    current_thread, foreground_thread, FALSE
                );
            }
        }
        if (!ensure_taskbar_ini(true)) {
            return 1;
        }
        return generate(default_configuration());
    }

    void wait_for_enter() {
        if (!_isatty(_fileno(stdin))) return;
        std::wcout << L"\nPress Enter to close taskbar_config.exe.";
        std::wcout.flush();
        std::wstring ignored;
        std::getline(std::wcin, ignored);
    }
}

int wmain(int argc, wchar_t* argv[]) {
    taskbar_config.connect_to_logger();
    taskbar_config.log_and_log("taskbar_config.exe started");
    try {
        if (ac::config::components_request::is_initialize_run(argc, argv)) {
            if (!ensure_taskbar_ini(false)) {
                taskbar_config.log_and_print(
                    "Taskbar configuration was not initialized."
                );
                return 1;
            }
            taskbar_config.log_and_log("taskbar.ini initialized");
            return ac::config::components_request::run_component_update(
                "taskbar"
            );
        }
        const int result = run();
        int exit_code = result;
        if (result == 0) {
            exit_code =
                ac::config::components_request::run_component_update("taskbar");
        }
        wait_for_enter();
        return exit_code;
    }
    catch (const std::exception& error) {
        std::cerr << "taskbar_config.exe failed: " << error.what() << '\n';
        taskbar_config.log_and_print(
            "taskbar_config.exe failed: {}",
            error.what()
        );
        wait_for_enter();
        return 1;
    }
}
