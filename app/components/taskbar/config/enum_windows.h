#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace taskbar_windows {
    struct IniDocument {
        std::unordered_map<std::string, std::string> values;

        std::string_view find(
            const std::string_view section,
            const std::string_view key
        ) const;
    };

    struct WindowMatcher {
        std::wstring process_name;
        std::wstring executable_path;
        std::wstring window_class;
        std::wstring window_application_id;

        bool configured() const noexcept {
            return !process_name.empty() || !executable_path.empty() ||
                !window_class.empty() || !window_application_id.empty();
        }
    };

    struct ApplicationConfiguration {
        std::filesystem::path path;
        std::string key;
        std::string application_id;
        WindowMatcher matcher;
        std::wstring fallback_executable_path;
        std::string activate_command;
    };

    struct WindowInfo {
        HWND handle {};
        DWORD process_id {};
        std::wstring title;
        std::wstring window_class;
        std::wstring process_path;
        std::wstring process_name;
        std::wstring application_id;
    };

    std::string_view trim(std::string_view value) noexcept;
    std::wstring_view trim(std::wstring_view value) noexcept;
    std::string ascii_lower(std::string_view value);
    std::optional<std::wstring> from_utf8(std::string_view value);
    std::optional<std::string> to_utf8(std::wstring_view value);
    bool equal_insensitive(std::wstring_view left, std::wstring_view right);
    bool contains_insensitive(std::wstring_view value, std::wstring_view part);

    std::optional<IniDocument> read_ini(const std::filesystem::path& path);
    std::vector<ApplicationConfiguration> load_applications(
        const std::filesystem::path& directory
    );

    bool matches(const WindowInfo& window, const WindowMatcher& matcher);
    unsigned match_count(
        const std::vector<WindowInfo>& windows,
        const WindowMatcher& matcher
    );
    std::optional<std::vector<WindowInfo>> enumerate_windows();
}
