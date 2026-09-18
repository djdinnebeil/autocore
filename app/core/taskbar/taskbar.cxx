module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Ole2.h>
#include <OleAuto.h>
#include <UIAutomation.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <wrl/client.h>

module auto_core.taskbar;

import std;
import auto_core.core.error;
import auto_core.core.keyboard;
import auto_core.core.paths;

using Microsoft::WRL::ComPtr;

namespace {
    constexpr std::uint32_t snapshot_schema_version = 7;
    constexpr std::uint32_t protocol_magic = 0x42544341; // ACTB
    constexpr std::uint32_t protocol_version = 7;
    constexpr std::uint32_t maximum_string_size = 64 * 1024;
    constexpr std::wstring_view pipe_path =
        LR"(\\.\pipe\AutoCore.Taskbar.v1)";
    constexpr std::wstring_view application_button_class =
        L"Taskbar.TaskListButtonAutomationPeer";
    constexpr std::wstring_view application_id_prefix = L"Appid: ";

    struct Slot {
        ac::taskbar::Position position;
        std::string automation_id;
        std::string application_id;
        std::string diagnostic_display_name;
    };

    enum class MultiWindowBehavior : std::uint8_t {
        one_shot,
        cycle
    };

    struct WindowMatcher {
        std::wstring process_name;
        std::wstring executable_path;
        std::wstring window_class;
        std::wstring window_application_id;

        [[nodiscard]] bool configured() const noexcept {
            return !process_name.empty() || !executable_path.empty() ||
                !window_class.empty() || !window_application_id.empty();
        }
    };

    struct ApplicationRoute {
        std::optional<ac::taskbar::Position> position;
        MultiWindowBehavior multi_window {MultiWindowBehavior::one_shot};
        WindowMatcher window_matcher;
    };

    struct SnapshotData {
        std::uint32_t schema_version {snapshot_schema_version};
        std::uint64_t generation {};
        ac::taskbar::SnapshotSource source {
            ac::taskbar::SnapshotSource::configured
        };
        std::vector<Slot> slots;
        std::unordered_map<std::string, ApplicationRoute> applications;
        std::vector<ac::taskbar::ConfiguredActivationCommand>
            activation_commands;
        std::unordered_map<std::string, std::wstring>
            fallback_executable_paths;
    };

    enum class Mode {
        live,
        cache
    };

    struct ApplicationConfiguration {
        std::string taskbar_application_id;
        std::vector<std::string> activation_commands;
        std::wstring fallback_executable_path;
        MultiWindowBehavior multi_window {MultiWindowBehavior::one_shot};
        WindowMatcher window_matcher;
    };

    struct Configuration {
        Mode mode {Mode::live};
        std::unordered_map<std::string, ac::taskbar::Position> fallback_positions;
        std::unordered_map<std::string, ApplicationConfiguration> applications;
    };

    struct WireHeader {
        std::uint32_t magic {};
        std::uint32_t protocol {};
        std::uint32_t schema {};
        std::uint32_t source {};
        std::uint64_t generation {};
        std::uint32_t slot_count {};
        std::uint32_t application_count {};
        std::uint32_t command_count {};
        std::uint32_t fallback_count {};
    };

    std::atomic<std::shared_ptr<const SnapshotData>> local_snapshot;
    std::atomic_bool authority_running {false};
    std::atomic_bool client_running {false};
    std::atomic_bool calculation_running {false};
    std::atomic_uint64_t next_generation {1};

    std::mutex snapshot_mutex;
    std::condition_variable snapshot_changed;

    std::mutex authority_threads_mutex;
    std::thread server_thread;
    std::thread calculation_thread;
    std::vector<std::thread> publisher_threads;

    std::mutex client_mutex;
    HANDLE client_pipe = INVALID_HANDLE_VALUE;
    std::thread client_thread;

    Configuration authority_configuration;

    std::string_view trim(const std::string_view value) noexcept {
        const auto first = value.find_first_not_of(" \t\r");
        if (first == std::string_view::npos) return {};
        const auto last = value.find_last_not_of(" \t\r");
        return value.substr(first, last - first + 1);
    }

    std::string ascii_lower(std::string_view value) {
        std::string result {value};
        std::ranges::transform(result, result.begin(), [](const unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return result;
    }

    std::string canonical_application_key(std::string key) {
        key = ascii_lower(key);
        if (key == "folder") return "file_explorer";
        if (key == "chrome") return "google_chrome";
        if (key == "visual") return "visual_studio";
        if (key == "vs_code") return "visual_studio_code";
        if (key == "zoom") return "zoom_workplace";
        return key;
    }

    void add_unique_command(
        std::vector<std::string>& commands,
        std::string command
    ) {
        if (command.empty()) return;
        if (std::ranges::find(commands, command) == commands.end()) {
            commands.push_back(std::move(command));
        }
    }

    void apply_historical_command_aliases(
        const std::string& key,
        std::vector<std::string>& commands
    ) {
        add_unique_command(commands, "activate_" + key);
        if (key == "file_explorer") {
            add_unique_command(commands, "activate_folder");
        }
        else if (key == "google_chrome") {
            add_unique_command(commands, "activate_chrome");
        }
        else if (key == "visual_studio") {
            add_unique_command(commands, "activate_visual");
        }
        else if (key == "visual_studio_code") {
            add_unique_command(commands, "activate_vs_code");
        }
        else if (key == "zoom_workplace") {
            add_unique_command(commands, "activate_zoom");
        }
    }

    std::string to_utf8(const std::wstring_view value) {
        if (value.empty()) return {};
        const int size = WideCharToMultiByte(
            CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
            nullptr, 0, nullptr, nullptr
        );
        if (size <= 0) return {};
        std::string result(static_cast<std::size_t>(size), '\0');
        if (WideCharToMultiByte(
                CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                result.data(), size, nullptr, nullptr
            ) <= 0) {
            return {};
        }
        return result;
    }

    std::wstring from_utf8(const std::string_view value) {
        if (value.empty()) return {};
        const int size = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), nullptr, 0
        );
        if (size <= 0) return {};
        std::wstring result(static_cast<std::size_t>(size), L'\0');
        if (MultiByteToWideChar(
                CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                static_cast<int>(value.size()), result.data(), size
            ) <= 0) {
            return {};
        }
        return result;
    }

    std::vector<std::string> parse_ascii_list(
        const std::string_view setting
    ) {
        std::vector<std::string> values;
        std::size_t first {};
        while (first <= setting.size()) {
            const std::size_t separator = setting.find('|', first);
            const std::string_view value = trim(setting.substr(
                first,
                separator == std::string_view::npos
                    ? std::string_view::npos
                    : separator - first
            ));
            if (!value.empty()) values.emplace_back(value);
            if (separator == std::string_view::npos) break;
            first = separator + 1;
        }
        return values;
    }

    std::wstring take_bstr(BSTR value) {
        if (value == nullptr) return {};
        std::wstring result(value, SysStringLen(value));
        SysFreeString(value);
        return result;
    }

    std::wstring current_name(IUIAutomationElement* element) {
        BSTR value {};
        return SUCCEEDED(element->get_CurrentName(&value))
            ? take_bstr(value)
            : std::wstring {};
    }

    std::wstring current_automation_id(IUIAutomationElement* element) {
        BSTR value {};
        return SUCCEEDED(element->get_CurrentAutomationId(&value))
            ? take_bstr(value)
            : std::wstring {};
    }

    std::optional<ac::taskbar::Position> configured_position(
        const int value
    ) {
        if (value >= 1 && value <= 10) {
            return ac::taskbar::Position {
                static_cast<std::uint8_t>(value)
            };
        }
        return std::nullopt;
    }

    void load_application_file(
        Configuration& config,
        const std::filesystem::path& path
    ) {
        std::ifstream input(path);
        if (!input) return;

        std::string application = canonical_application_key(path.stem().string());
        ApplicationConfiguration definition;
        std::string section;
        std::string line;

        while (std::getline(input, line)) {
            const std::string_view value = trim(line);
            if (value.empty() || value.starts_with(';') ||
                value.starts_with('#')) {
                continue;
            }
            if (value.starts_with('[') && value.ends_with(']')) {
                section = ascii_lower(trim(
                    value.substr(1, value.size() - 2)
                ));
                continue;
            }
            const auto equals = value.find('=');
            if (equals == std::string_view::npos) continue;
            const std::string key = ascii_lower(trim(value.substr(0, equals)));
            const std::string setting {trim(value.substr(equals + 1))};

            if (section == "application" && key == "key" &&
                !setting.empty()) {
                application = canonical_application_key(setting);
            }
            else if (section == "taskbar" &&
                key == "application_id") {
                definition.taskbar_application_id = setting;
            }
            else if (section == "window" && key == "process_name") {
                definition.window_matcher.process_name = from_utf8(setting);
            }
            else if (section == "window" && key == "executable_path") {
                definition.window_matcher.executable_path = from_utf8(setting);
            }
            else if (section == "window" && key == "class") {
                definition.window_matcher.window_class = from_utf8(setting);
            }
            else if (section == "window" && key == "application_id") {
                definition.window_matcher.window_application_id =
                    from_utf8(setting);
            }
            else if (section == "activation" && key == "multi_window") {
                definition.multi_window = ascii_lower(setting) == "cycle"
                    ? MultiWindowBehavior::cycle
                    : MultiWindowBehavior::one_shot;
            }
            else if (section == "commands" && key == "activate") {
                definition.activation_commands = parse_ascii_list(setting);
            }
            else if (section == "fallback" && key == "executable_path") {
                definition.fallback_executable_path = from_utf8(setting);
            }
        }

        if (!application.empty()) {
            apply_historical_command_aliases(
                application, definition.activation_commands
            );
            auto& configured = config.applications[application];
            if (!definition.taskbar_application_id.empty()) {
                configured.taskbar_application_id =
                    std::move(definition.taskbar_application_id);
            }
            if (!definition.activation_commands.empty()) {
                configured.activation_commands =
                    std::move(definition.activation_commands);
            }
            if (!definition.fallback_executable_path.empty()) {
                configured.fallback_executable_path =
                    std::move(definition.fallback_executable_path);
            }
            configured.multi_window = definition.multi_window;
            configured.window_matcher =
                std::move(definition.window_matcher);
        }
    }

    void load_application_files(Configuration& config) {
        const auto directory = ac::paths::taskbar_applications_directory();
        std::error_code error;
        if (!std::filesystem::is_directory(directory, error)) return;

        std::vector<std::filesystem::path> files;
        for (std::filesystem::directory_iterator iterator(directory, error);
             !error && iterator != std::filesystem::directory_iterator {};
             iterator.increment(error)) {
            if (!iterator->is_regular_file(error)) continue;
            if (ascii_lower(iterator->path().extension().string()) == ".ini") {
                files.push_back(iterator->path());
            }
        }
        std::ranges::sort(files);
        for (const auto& file : files) load_application_file(config, file);
    }

    std::filesystem::path positions_cache_file() {
        return ac::paths::taskbar_directory() / "cached_positions.ini";
    }

    void parse_position_assignment(
        Configuration& config,
        const std::string_view key,
        const std::string_view setting
    ) {
        int digit = -1;
        const auto converted = std::from_chars(
            setting.data(), setting.data() + setting.size(), digit
        );
        if (converted.ec != std::errc {} ||
            converted.ptr != setting.data() + setting.size()) {
            return;
        }
        if (const auto position = configured_position(digit)) {
            config.fallback_positions.insert_or_assign(
                canonical_application_key(std::string {key}), *position
            );
        }
    }

    void load_positions_cache(Configuration& config) {
        std::ifstream input(positions_cache_file());
        if (!input) return;

        std::string section;
        std::string line;
        while (std::getline(input, line)) {
            const std::string_view value = trim(line);
            if (value.empty() || value.starts_with(';') ||
                value.starts_with('#')) {
                continue;
            }
            if (value.starts_with('[') && value.ends_with(']')) {
                section = ascii_lower(trim(value.substr(1, value.size() - 2)));
                continue;
            }
            const auto equals = value.find('=');
            if (equals == std::string_view::npos) continue;
            const std::string key = ascii_lower(trim(value.substr(0, equals)));
            const std::string setting {trim(value.substr(equals + 1))};
            if (key.empty()) continue;
            if (section.empty() || section == "taskbar") {
                parse_position_assignment(config, key, setting);
            }
        }
    }

    void store_cached_positions(
        Configuration& config,
        const SnapshotData& snapshot
    ) {
        config.fallback_positions.clear();
        for (const auto& [application, route] : snapshot.applications) {
            if (route.position && route.position->valid()) {
                config.fallback_positions.insert_or_assign(
                    application, *route.position
                );
            }
        }
    }

    void write_positions_cache(const Configuration& config) {
        std::vector<std::pair<std::uint8_t, std::string>> rows;
        for (const auto& [application, position] : config.fallback_positions) {
            if (!position.valid()) continue;
            rows.emplace_back(position.value, application);
        }
        std::ranges::sort(rows);

        std::string contents {"[taskbar]\n"};
        for (const auto& [position, application] : rows) {
            contents += application;
            contents += " = ";
            contents += std::to_string(position);
            contents += '\n';
        }

        const auto destination = positions_cache_file();
        std::error_code error;
        std::filesystem::create_directories(destination.parent_path(), error);
        std::filesystem::path temporary = destination;
        temporary += ".tmp";
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) return;
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.close();
        if (!output) {
            std::filesystem::remove(temporary, error);
            return;
        }
        if (!MoveFileExW(
                temporary.c_str(),
                destination.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
            )) {
            std::filesystem::remove(temporary, error);
        }
    }

    Configuration load_configuration() {
        Configuration config;
        std::ifstream input(ac::paths::config_directory() / "taskbar.ini");
        if (input) {
            std::string section;
            std::string line;
            while (std::getline(input, line)) {
                const std::string_view value = trim(line);
                if (value.empty() || value.starts_with(';') ||
                    value.starts_with('#')) {
                    continue;
                }
                if (value.starts_with('[') && value.ends_with(']')) {
                    section = ascii_lower(trim(
                        value.substr(1, value.size() - 2)
                    ));
                    continue;
                }
                const auto equals = value.find('=');
                if (equals == std::string_view::npos) continue;
                const std::string key =
                    ascii_lower(trim(value.substr(0, equals)));
                const std::string setting {trim(value.substr(equals + 1))};
                if (key.empty()) continue;

                if (section == "taskbar" && key == "mode") {
                    config.mode = ascii_lower(setting) == "cache"
                        ? Mode::cache
                        : Mode::live;
                }
            }
        }
        load_application_files(config);
        load_positions_cache(config);
        return config;
    }

    ApplicationRoute application_route(
        const Configuration& config,
        const std::string_view application,
        const std::optional<ac::taskbar::Position> position
    ) {
        ApplicationRoute route {.position = position};
        const auto configured = config.applications.find(
            std::string {application}
        );
        if (configured != config.applications.end()) {
            route.multi_window = configured->second.multi_window;
            route.window_matcher = configured->second.window_matcher;
        }
        return route;
    }

    void add_unmapped_configured_applications(
        SnapshotData& snapshot,
        const Configuration& config
    ) {
        for (const auto& [application, definition] : config.applications) {
            if (snapshot.applications.contains(application)) continue;
            snapshot.applications.insert_or_assign(
                application,
                ApplicationRoute {
                    .position = std::nullopt,
                    .multi_window = definition.multi_window,
                    .window_matcher = definition.window_matcher
                }
            );
        }
    }

    std::shared_ptr<SnapshotData> configured_snapshot(
        const Configuration& config,
        const ac::taskbar::SnapshotSource source,
        const bool include_fallback_positions = true
    ) {
        auto snapshot = std::make_shared<SnapshotData>();
        snapshot->generation = next_generation.fetch_add(1);
        snapshot->source = source;
        for (const auto& [application, definition] : config.applications) {
            for (const std::string& command :
                 definition.activation_commands) {
                snapshot->activation_commands.push_back(
                    ac::taskbar::ConfiguredActivationCommand {
                        .command = command,
                        .application = application
                    }
                );
            }
            if (!definition.fallback_executable_path.empty()) {
                snapshot->fallback_executable_paths.insert_or_assign(
                    application, definition.fallback_executable_path
                );
            }
        }
        if (include_fallback_positions) {
            for (const auto& [application, position] :
                 config.fallback_positions) {
                snapshot->applications.insert_or_assign(
                    application,
                    application_route(config, application, position)
                );
            }
        }
        add_unmapped_configured_applications(*snapshot, config);
        return snapshot;
    }

    std::expected<std::vector<Slot>, std::string> calculate_first_ten() {
        const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(initialized)) {
            return std::unexpected(std::format(
                "CoInitializeEx failed: 0x{:08X}",
                static_cast<unsigned int>(initialized)
            ));
        }
        struct ComGuard { ~ComGuard() { CoUninitialize(); } } guard;

        const HWND taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
        if (taskbar == nullptr) {
            return std::unexpected("Shell_TrayWnd was not found");
        }

        ComPtr<IUIAutomation> automation;
        HRESULT result = CoCreateInstance(
            CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&automation)
        );
        if (FAILED(result)) {
            return std::unexpected("Unable to create UI Automation");
        }

        ComPtr<IUIAutomationElement> root;
        result = automation->ElementFromHandle(taskbar, &root);
        if (FAILED(result) || root == nullptr) {
            return std::unexpected("Unable to obtain the taskbar UIA root");
        }

        VARIANT class_value;
        VariantInit(&class_value);
        class_value.vt = VT_BSTR;
        class_value.bstrVal = SysAllocString(application_button_class.data());
        if (class_value.bstrVal == nullptr) {
            return std::unexpected("Unable to allocate the UIA class condition");
        }

        ComPtr<IUIAutomationCondition> class_condition;
        result = automation->CreatePropertyCondition(
            UIA_ClassNamePropertyId, class_value, &class_condition
        );
        VariantClear(&class_value);
        if (FAILED(result)) {
            return std::unexpected("Unable to create the UIA class condition");
        }

        ComPtr<IUIAutomationElement> current;
        result = root->FindFirst(
            TreeScope_Descendants, class_condition.Get(), &current
        );
        if (FAILED(result) || current == nullptr) {
            return std::unexpected("No taskbar application button was found");
        }

        ComPtr<IUIAutomationTreeWalker> walker;
        result = automation->get_RawViewWalker(&walker);
        if (FAILED(result)) {
            return std::unexpected("Unable to create the raw UIA tree walker");
        }

        std::vector<Slot> slots;
        slots.reserve(10);
        while (current != nullptr && slots.size() < 10) {
            const std::wstring automation_id =
                current_automation_id(current.Get());
            if (automation_id.starts_with(application_id_prefix)) {
                slots.push_back(Slot {
                    .position = ac::taskbar::Position {
                        static_cast<std::uint8_t>(slots.size() + 1)
                    },
                    .automation_id = to_utf8(automation_id),
                    .application_id = to_utf8(automation_id.substr(
                        application_id_prefix.size()
                    )),
                    .diagnostic_display_name = to_utf8(
                        current_name(current.Get())
                    )
                });
            }

            ComPtr<IUIAutomationElement> next;
            if (FAILED(walker->GetNextSiblingElement(current.Get(), &next))) {
                break;
            }
            current = std::move(next);
        }
        return slots;
    }

    std::expected<
        std::vector<ac::taskbar::DiscoveredTaskbarApplication>,
        std::string
    > calculate_all_taskbar_applications() {
        const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(initialized)) {
            return std::unexpected(std::format(
                "CoInitializeEx failed: 0x{:08X}",
                static_cast<unsigned int>(initialized)
            ));
        }
        struct ComGuard { ~ComGuard() { CoUninitialize(); } } guard;

        const HWND taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
        if (taskbar == nullptr) {
            return std::unexpected("Shell_TrayWnd was not found");
        }

        ComPtr<IUIAutomation> automation;
        HRESULT result = CoCreateInstance(
            CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&automation)
        );
        if (FAILED(result)) {
            return std::unexpected("Unable to create UI Automation");
        }

        ComPtr<IUIAutomationElement> root;
        result = automation->ElementFromHandle(taskbar, &root);
        if (FAILED(result) || root == nullptr) {
            return std::unexpected("Unable to obtain the taskbar UIA root");
        }

        VARIANT class_value;
        VariantInit(&class_value);
        class_value.vt = VT_BSTR;
        class_value.bstrVal = SysAllocString(application_button_class.data());
        if (class_value.bstrVal == nullptr) {
            return std::unexpected("Unable to allocate the UIA class condition");
        }

        ComPtr<IUIAutomationCondition> class_condition;
        result = automation->CreatePropertyCondition(
            UIA_ClassNamePropertyId, class_value, &class_condition
        );
        VariantClear(&class_value);
        if (FAILED(result)) {
            return std::unexpected("Unable to create the UIA class condition");
        }

        ComPtr<IUIAutomationElement> current;
        result = root->FindFirst(
            TreeScope_Descendants, class_condition.Get(), &current
        );
        if (FAILED(result) || current == nullptr) {
            return std::unexpected("No taskbar application button was found");
        }

        ComPtr<IUIAutomationTreeWalker> walker;
        result = automation->get_RawViewWalker(&walker);
        if (FAILED(result)) {
            return std::unexpected("Unable to create the raw UIA tree walker");
        }

        constexpr std::size_t maximum_buttons = 4096;
        std::vector<ac::taskbar::DiscoveredTaskbarApplication> applications;
        while (current != nullptr && applications.size() < maximum_buttons) {
            const std::wstring automation_id =
                current_automation_id(current.Get());
            if (automation_id.starts_with(application_id_prefix)) {
                const std::size_t ordinal = applications.size() + 1;
                applications.push_back(
                    ac::taskbar::DiscoveredTaskbarApplication {
                        .ordinal = ordinal,
                        .native_position = ordinal <= 10
                            ? std::optional<ac::taskbar::Position> {
                                ac::taskbar::Position {
                                    static_cast<std::uint8_t>(ordinal)
                                }
                            }
                            : std::nullopt,
                        .automation_id = to_utf8(automation_id),
                        .application_id = to_utf8(automation_id.substr(
                            application_id_prefix.size()
                        )),
                        .display_name = to_utf8(current_name(current.Get()))
                    }
                );
            }

            ComPtr<IUIAutomationElement> next;
            if (FAILED(walker->GetNextSiblingElement(current.Get(), &next))) {
                break;
            }
            current = std::move(next);
        }
        return applications;
    }

    bool application_ids_equal(
        const std::string_view left,
        const std::string_view right
    ) {
        return ascii_lower(left) == ascii_lower(right);
    }

    std::shared_ptr<SnapshotData> live_snapshot(
        const Configuration& config,
        std::vector<Slot> slots
    ) {
        auto snapshot = configured_snapshot(
            config, ac::taskbar::SnapshotSource::live, false
        );
        snapshot->slots = std::move(slots);

        for (const auto& [application, definition] : config.applications) {
            const std::string& configured_id =
                definition.taskbar_application_id;
            if (configured_id.empty()) continue;
            const auto found = std::ranges::find_if(
                snapshot->slots,
                [&](const Slot& slot) {
                    return application_ids_equal(
                        slot.application_id, configured_id
                    ) || application_ids_equal(
                        slot.automation_id, configured_id
                    );
                }
            );
            if (found != snapshot->slots.end()) {
                snapshot->applications.insert_or_assign(
                    application,
                    application_route(config, application, found->position)
                );
            }
        }

        add_unmapped_configured_applications(*snapshot, config);
        return snapshot;
    }

    void publish(std::shared_ptr<const SnapshotData> snapshot) {
        local_snapshot.store(std::move(snapshot));
        snapshot_changed.notify_all();
    }

    bool write_bytes(HANDLE pipe, const void* data, std::size_t size) {
        const auto* cursor = static_cast<const std::byte*>(data);
        while (size != 0) {
            const DWORD request = static_cast<DWORD>((std::min)(
                size,
                static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())
            ));
            DWORD written {};
            if (!WriteFile(pipe, cursor, request, &written, nullptr) ||
                written == 0) {
                return false;
            }
            cursor += written;
            size -= written;
        }
        return true;
    }

    bool read_bytes(HANDLE pipe, void* data, std::size_t size) {
        auto* cursor = static_cast<std::byte*>(data);
        while (size != 0) {
            const DWORD request = static_cast<DWORD>((std::min)(
                size,
                static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())
            ));
            DWORD read {};
            if (!ReadFile(pipe, cursor, request, &read, nullptr) || read == 0) {
                return false;
            }
            cursor += read;
            size -= read;
        }
        return true;
    }

    bool write_string(HANDLE pipe, const std::string_view value) {
        if (value.size() > maximum_string_size) return false;
        const auto size = static_cast<std::uint32_t>(value.size());
        return write_bytes(pipe, &size, sizeof(size)) &&
            write_bytes(pipe, value.data(), value.size());
    }

    std::optional<std::string> read_string(HANDLE pipe) {
        std::uint32_t size {};
        if (!read_bytes(pipe, &size, sizeof(size)) ||
            size > maximum_string_size) {
            return std::nullopt;
        }
        std::string value(size, '\0');
        if (!read_bytes(pipe, value.data(), value.size())) {
            return std::nullopt;
        }
        return value;
    }

    bool write_wide_string(HANDLE pipe, const std::wstring_view value) {
        return write_string(pipe, to_utf8(value));
    }

    std::optional<std::wstring> read_wide_string(HANDLE pipe) {
        const auto value = read_string(pipe);
        return value
            ? std::optional<std::wstring> {from_utf8(*value)}
            : std::nullopt;
    }

    bool write_snapshot(HANDLE pipe, const SnapshotData& snapshot) {
        if (snapshot.slots.size() > 10 ||
            snapshot.applications.size() >
                (std::numeric_limits<std::uint32_t>::max)() ||
            snapshot.activation_commands.size() >
                (std::numeric_limits<std::uint32_t>::max)() ||
            snapshot.fallback_executable_paths.size() >
                (std::numeric_limits<std::uint32_t>::max)()) {
            return false;
        }
        const WireHeader header {
            .magic = protocol_magic,
            .protocol = protocol_version,
            .schema = snapshot.schema_version,
            .source = static_cast<std::uint32_t>(snapshot.source),
            .generation = snapshot.generation,
            .slot_count = static_cast<std::uint32_t>(snapshot.slots.size()),
            .application_count = static_cast<std::uint32_t>(
                snapshot.applications.size()
            ),
            .command_count = static_cast<std::uint32_t>(
                snapshot.activation_commands.size()
            ),
            .fallback_count = static_cast<std::uint32_t>(
                snapshot.fallback_executable_paths.size()
            )
        };
        if (!write_bytes(pipe, &header, sizeof(header))) return false;

        for (const Slot& slot : snapshot.slots) {
            if (!write_bytes(pipe, &slot.position.value,
                    sizeof(slot.position.value)) ||
                !write_string(pipe, slot.automation_id) ||
                !write_string(pipe, slot.application_id) ||
                !write_string(pipe, slot.diagnostic_display_name)) {
                return false;
            }
        }
        for (const auto& [application, route] : snapshot.applications) {
            const auto behavior = static_cast<std::uint8_t>(
                route.multi_window
            );
            const std::uint8_t position =
                route.position ? route.position->value : std::uint8_t {0};
            if (!write_string(pipe, application) ||
                !write_bytes(pipe, &position, sizeof(position)) ||
                !write_bytes(pipe, &behavior, sizeof(behavior)) ||
                !write_wide_string(
                    pipe, route.window_matcher.process_name
                ) ||
                !write_wide_string(
                    pipe, route.window_matcher.executable_path
                ) ||
                !write_wide_string(
                    pipe, route.window_matcher.window_class
                ) ||
                !write_wide_string(
                    pipe, route.window_matcher.window_application_id
                )) {
                return false;
            }
        }
        for (const auto& command : snapshot.activation_commands) {
            if (!write_string(pipe, command.command) ||
                !write_string(pipe, command.application)) {
                return false;
            }
        }
        for (const auto& [application, executable] :
             snapshot.fallback_executable_paths) {
            if (!write_string(pipe, application) ||
                !write_wide_string(pipe, executable)) {
                return false;
            }
        }
        return true;
    }

    std::shared_ptr<SnapshotData> read_snapshot(HANDLE pipe) {
        WireHeader header {};
        if (!read_bytes(pipe, &header, sizeof(header)) ||
            header.magic != protocol_magic ||
            header.protocol != protocol_version ||
            header.schema != snapshot_schema_version ||
            header.slot_count > 10 ||
            header.application_count > 4096 ||
            header.command_count > 4096 ||
            header.fallback_count > 4096) {
            return {};
        }

        auto snapshot = std::make_shared<SnapshotData>();
        snapshot->schema_version = header.schema;
        snapshot->generation = header.generation;
        snapshot->source = static_cast<ac::taskbar::SnapshotSource>(
            header.source
        );
        snapshot->slots.reserve(header.slot_count);
        for (std::uint32_t index = 0; index < header.slot_count; ++index) {
            std::uint8_t position {};
            if (!read_bytes(pipe, &position, sizeof(position))) return {};
            auto automation_id = read_string(pipe);
            auto application_id = read_string(pipe);
            auto name = read_string(pipe);
            const ac::taskbar::Position logical {position};
            if (!automation_id || !application_id || !name ||
                !logical.valid()) {
                return {};
            }
            snapshot->slots.push_back(Slot {
                .position = logical,
                .automation_id = std::move(*automation_id),
                .application_id = std::move(*application_id),
                .diagnostic_display_name = std::move(*name)
            });
        }
        for (std::uint32_t index = 0;
             index < header.application_count; ++index) {
            auto application = read_string(pipe);
            std::uint8_t position {};
            std::uint8_t behavior {};
            if (!application ||
                !read_bytes(pipe, &position, sizeof(position)) ||
                !read_bytes(pipe, &behavior, sizeof(behavior))) {
                return {};
            }
            std::optional<ac::taskbar::Position> logical;
            if (position != 0) {
                logical = ac::taskbar::Position {position};
                if (!logical->valid()) return {};
            }
            if (behavior > static_cast<std::uint8_t>(
                    MultiWindowBehavior::cycle
                )) {
                return {};
            }
            auto process_name = read_wide_string(pipe);
            auto executable_path = read_wide_string(pipe);
            auto window_class = read_wide_string(pipe);
            auto window_application_id = read_wide_string(pipe);
            if (!process_name || !executable_path || !window_class ||
                !window_application_id) {
                return {};
            }
            snapshot->applications.insert_or_assign(
                ascii_lower(*application),
                ApplicationRoute {
                    .position = logical,
                    .multi_window = static_cast<MultiWindowBehavior>(behavior),
                    .window_matcher = WindowMatcher {
                        .process_name = std::move(*process_name),
                        .executable_path = std::move(*executable_path),
                        .window_class = std::move(*window_class),
                        .window_application_id =
                            std::move(*window_application_id)
                    }
                }
            );
        }
        snapshot->activation_commands.reserve(header.command_count);
        for (std::uint32_t index = 0; index < header.command_count; ++index) {
            auto command = read_string(pipe);
            auto application = read_string(pipe);
            if (!command || command->empty() || !application ||
                application->empty()) {
                return {};
            }
            snapshot->activation_commands.push_back(
                ac::taskbar::ConfiguredActivationCommand {
                    .command = std::move(*command),
                    .application = ascii_lower(*application)
                }
            );
        }
        for (std::uint32_t index = 0;
             index < header.fallback_count; ++index) {
            auto application = read_string(pipe);
            auto executable = read_wide_string(pipe);
            if (!application || application->empty() || !executable ||
                executable->empty()) {
                return {};
            }
            snapshot->fallback_executable_paths.insert_or_assign(
                ascii_lower(*application), std::move(*executable)
            );
        }
        return snapshot;
    }

    void publish_to_client(HANDLE pipe) {
        std::uint64_t last_generation {};
        while (authority_running.load()) {
            std::shared_ptr<const SnapshotData> snapshot;
            {
                std::unique_lock lock(snapshot_mutex);
                snapshot_changed.wait(lock, [&] {
                    const auto current = local_snapshot.load();
                    return !authority_running.load() ||
                        (current && current->generation > last_generation);
                });
                if (!authority_running.load()) break;
                snapshot = local_snapshot.load();
            }
            if (!snapshot || !write_snapshot(pipe, *snapshot)) break;
            last_generation = snapshot->generation;
        }
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }

    void run_server() {
        while (authority_running.load()) {
            HANDLE pipe = CreateNamedPipeW(
                pipe_path.data(),
                PIPE_ACCESS_OUTBOUND,
                PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                4096, 4096, 0, nullptr
            );
            if (pipe == INVALID_HANDLE_VALUE) break;

            const BOOL connected = ConnectNamedPipe(pipe, nullptr);
            if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) {
                CloseHandle(pipe);
                if (!authority_running.load()) break;
                continue;
            }

            std::scoped_lock lock(authority_threads_mutex);
            publisher_threads.emplace_back(publish_to_client, pipe);
        }
    }

    void calculate_and_publish() {
        const auto calculated = calculate_first_ten();
        if (!calculated) {
            ac::error::log(std::format(
                "Unable to calculate taskbar positions: {}\n",
                calculated.error()
            ));
            publish(configured_snapshot(
                authority_configuration,
                ac::taskbar::SnapshotSource::configured
            ));
        }
        else {
            auto snapshot = live_snapshot(
                authority_configuration,
                std::move(*calculated)
            );
            store_cached_positions(authority_configuration, *snapshot);
            write_positions_cache(authority_configuration);
            publish(std::move(snapshot));
        }
        calculation_running.store(false);
    }

    bool start_calculation() {
        bool expected = false;
        if (!calculation_running.compare_exchange_strong(expected, true)) {
            return false;
        }

        std::scoped_lock lock(authority_threads_mutex);
        if (calculation_thread.joinable()) calculation_thread.join();
        calculation_thread = std::thread(calculate_and_publish);
        return true;
    }

    void listen_for_snapshots() {
        while (client_running.load()) {
            HANDLE pipe = INVALID_HANDLE_VALUE;
            {
                std::scoped_lock lock(client_mutex);
                pipe = client_pipe;
            }
            const auto snapshot = read_snapshot(pipe);
            if (!snapshot) break;
            const auto current = local_snapshot.load();
            if (!current || snapshot->generation > current->generation) {
                local_snapshot.store(snapshot);
            }
        }
        client_running.store(false);
    }

    HANDLE connect_pipe(const std::chrono::milliseconds timeout) {
        const ULONGLONG deadline = GetTickCount64() +
            static_cast<ULONGLONG>((std::max)(timeout.count(), 0LL));
        while (true) {
            HANDLE pipe = CreateFileW(
                pipe_path.data(), GENERIC_READ, 0, nullptr,
                OPEN_EXISTING, 0, nullptr
            );
            if (pipe != INVALID_HANDLE_VALUE) return pipe;

            const DWORD error = GetLastError();
            if (error != ERROR_PIPE_BUSY && error != ERROR_FILE_NOT_FOUND) {
                return INVALID_HANDLE_VALUE;
            }
            const ULONGLONG now = GetTickCount64();
            if (now >= deadline) return INVALID_HANDLE_VALUE;
            const DWORD delay = static_cast<DWORD>((std::min)(
                deadline - now, static_cast<ULONGLONG>(50)
            ));
            if (error == ERROR_PIPE_BUSY) {
                WaitNamedPipeW(pipe_path.data(), delay);
            }
            else {
                Sleep(delay);
            }
        }
    }

    bool equal_case_insensitive(
        const std::wstring_view left,
        const std::wstring_view right
    ) {
        return left.size() <= (std::numeric_limits<int>::max)() &&
            right.size() <= (std::numeric_limits<int>::max)() &&
            CompareStringOrdinal(
                left.data(), static_cast<int>(left.size()),
                right.data(), static_cast<int>(right.size()), TRUE
            ) == CSTR_EQUAL;
    }

    std::wstring window_title(const HWND window) {
        const int length = GetWindowTextLengthW(window);
        if (length <= 0) return {};
        std::wstring value(static_cast<std::size_t>(length) + 1, L'\0');
        const int copied = GetWindowTextW(
            window, value.data(), static_cast<int>(value.size())
        );
        if (copied <= 0) return {};
        value.resize(static_cast<std::size_t>(copied));
        return value;
    }

    std::wstring window_class(const HWND window) {
        std::wstring value(256, L'\0');
        const int copied = GetClassNameW(
            window, value.data(), static_cast<int>(value.size())
        );
        if (copied <= 0) return {};
        value.resize(static_cast<std::size_t>(copied));
        return value;
    }

    std::wstring process_path(const DWORD process_id) {
        if (process_id == 0) return {};
        const HANDLE process = OpenProcess(
            PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id
        );
        if (process == nullptr) return {};
        struct ProcessGuard {
            HANDLE value;
            ~ProcessGuard() { CloseHandle(value); }
        } guard {process};

        std::wstring value(32'768, L'\0');
        DWORD size = static_cast<DWORD>(value.size());
        if (!QueryFullProcessImageNameW(
                process, 0, value.data(), &size
            )) {
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

    bool window_matches(
        const HWND window,
        const WindowMatcher& matcher,
        std::unordered_map<DWORD, std::wstring>& process_paths
    ) {
        if (!matcher.configured() || !IsWindowVisible(window)) return false;
        if (window_title(window).empty()) return false;

        if (!matcher.window_class.empty() &&
            !equal_case_insensitive(
                window_class(window), matcher.window_class
            )) {
            return false;
        }

        if (!matcher.window_application_id.empty() &&
            !equal_case_insensitive(
                window_application_id(window), matcher.window_application_id
            )) {
            return false;
        }

        if (!matcher.process_name.empty() ||
            !matcher.executable_path.empty()) {
            DWORD process_id {};
            GetWindowThreadProcessId(window, &process_id);
            auto [path, inserted] = process_paths.try_emplace(process_id);
            if (inserted) path->second = process_path(process_id);
            if (path->second.empty()) return false;
            if (!matcher.executable_path.empty() &&
                !equal_case_insensitive(
                    path->second, matcher.executable_path
                )) {
                return false;
            }
            if (!matcher.process_name.empty() &&
                !equal_case_insensitive(
                    std::filesystem::path {path->second}.filename().wstring(),
                    matcher.process_name
                )) {
                return false;
            }
        }
        return true;
    }

    struct WindowListContext {
        const WindowMatcher* matcher {};
        std::vector<HWND> windows;
        std::unordered_map<DWORD, std::wstring> process_paths;
        std::uint8_t stop_after {};
    };

    BOOL CALLBACK collect_matching_windows(
        const HWND window,
        const LPARAM parameter
    ) {
        auto& context = *reinterpret_cast<WindowListContext*>(parameter);
        if (window_matches(
                window, *context.matcher, context.process_paths
            )) {
            context.windows.push_back(window);
            if (context.stop_after != 0 &&
                context.windows.size() >= context.stop_after) {
                return FALSE;
            }
        }
        return TRUE;
    }

    std::vector<HWND> matching_window_list(const WindowMatcher& matcher) {
        if (!matcher.configured()) return {};
        WindowListContext context {.matcher = &matcher};
        EnumWindows(
            collect_matching_windows,
            reinterpret_cast<LPARAM>(&context)
        );
        return context.windows;
    }

    std::uint8_t matching_window_count(const WindowMatcher& matcher) {
        if (!matcher.configured()) return 0;
        WindowListContext context {
            .matcher = &matcher,
            .stop_after = 2
        };
        EnumWindows(
            collect_matching_windows,
            reinterpret_cast<LPARAM>(&context)
        );
        return static_cast<std::uint8_t>(context.windows.size());
    }

    WindowMatcher compiled_default_matcher(const std::string_view application) {
        if (application == "wordpad") {
            return WindowMatcher {.process_name = L"wordpad.exe"};
        }
        if (application == "powershell_admin") {
            return WindowMatcher {.process_name = L"powershell.exe"};
        }
        return {};
    }

    void write_taskbar_keymap_manifest(const Configuration& config) {
        std::set<std::string> commands {
            "activate_auto_core",
            "activate_powershell_in_admin",
            "activate_wordpad",
            "launch_gitbash",
            "launch_powershell",
            "refresh_taskbar_positions"
        };
        for (const auto& [application, definition] : config.applications) {
            for (const std::string& command : definition.activation_commands) {
                commands.insert(command);
            }
        }

        std::string contents;
        for (const std::string& command : commands) {
            contents += command;
            contents += '\n';
        }

        const auto destination =
            ac::paths::keymap_components_directory() / "taskbar.keymap_commands.txt";
        std::error_code error;
        std::filesystem::create_directories(destination.parent_path(), error);
        std::filesystem::path temporary = destination;
        temporary += ".tmp";
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) return;
        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size())
        );
        output.close();
        if (!output) {
            std::filesystem::remove(temporary, error);
            return;
        }
        if (!MoveFileExW(
                temporary.c_str(),
                destination.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
            )) {
            std::filesystem::remove(temporary, error);
        }
    }
}

namespace ac::taskbar {
    bool start_authority() {
        bool expected = false;
        if (!authority_running.compare_exchange_strong(expected, true)) {
            return true;
        }

        authority_configuration = load_configuration();
        write_taskbar_keymap_manifest(authority_configuration);
        server_thread = std::thread(run_server);

        if (authority_configuration.mode == Mode::cache &&
            !authority_configuration.fallback_positions.empty()) {
            publish(configured_snapshot(
                authority_configuration, SnapshotSource::configured
            ));
            return true;
        }
        return start_calculation();
    }

    bool wait_for_initial_snapshot(const std::chrono::milliseconds timeout) {
        std::unique_lock lock(snapshot_mutex);
        return snapshot_changed.wait_for(lock, timeout, [] {
            const auto snapshot = local_snapshot.load();
            return snapshot && snapshot->generation != 0;
        });
    }

    bool request_refresh() {
        if (!authority_running.load()) return false;
        return start_calculation();
    }

    void stop_authority() noexcept {
        if (!authority_running.exchange(false)) return;
        snapshot_changed.notify_all();

        if (server_thread.joinable()) {
            CancelSynchronousIo(server_thread.native_handle());
            server_thread.join();
        }

        std::scoped_lock lock(authority_threads_mutex);
        if (calculation_thread.joinable()) calculation_thread.join();
        for (std::thread& thread : publisher_threads) {
            if (thread.joinable()) thread.join();
        }
        publisher_threads.clear();
    }

    bool connect(const std::chrono::milliseconds timeout) {
        bool expected = false;
        if (!client_running.compare_exchange_strong(expected, true)) {
            return true;
        }

        HANDLE pipe = connect_pipe(timeout);
        if (pipe == INVALID_HANDLE_VALUE) {
            client_running.store(false);
            return false;
        }

        const auto initial = read_snapshot(pipe);
        if (!initial) {
            CloseHandle(pipe);
            client_running.store(false);
            return false;
        }
        local_snapshot.store(initial);
        {
            std::scoped_lock lock(client_mutex);
            client_pipe = pipe;
        }
        client_thread = std::thread(listen_for_snapshots);
        return true;
    }

    void disconnect() noexcept {
        client_running.store(false);
        HANDLE pipe = INVALID_HANDLE_VALUE;
        {
            std::scoped_lock lock(client_mutex);
            pipe = client_pipe;
            client_pipe = INVALID_HANDLE_VALUE;
        }
        if (pipe != INVALID_HANDLE_VALUE) {
            (void)CancelIoEx(pipe, nullptr);
            (void)CloseHandle(pipe);
        }
        if (client_thread.joinable()) {
            (void)CancelSynchronousIo(client_thread.native_handle());
            client_thread.join();
        }
    }

    std::optional<Position> get_native_taskbar_position(
        const std::string_view application
    ) {
        const auto snapshot = local_snapshot.load();
        if (!snapshot || snapshot->source == SnapshotSource::disabled) {
            return std::nullopt;
        }
        const auto found = snapshot->applications.find(
            canonical_application_key(std::string {application})
        );
        if (found == snapshot->applications.end() ||
            !found->second.position ||
            !found->second.position->valid()) {
            return std::nullopt;
        }
        return found->second.position;
    }

    std::vector<ConfiguredActivationCommand>
    configured_activation_commands() {
        const auto snapshot = local_snapshot.load();
        if (!snapshot) return {};
        auto commands = snapshot->activation_commands;
        std::ranges::sort(commands, {}, [](const auto& command) {
            return std::tie(command.command, command.application);
        });
        return commands;
    }

    std::optional<std::wstring> configured_fallback_executable_path(
        const std::string_view application
    ) {
        const auto snapshot = local_snapshot.load();
        if (!snapshot) return std::nullopt;
        const auto found = snapshot->fallback_executable_paths.find(
            canonical_application_key(std::string {application})
        );
        return found == snapshot->fallback_executable_paths.end()
            ? std::nullopt
            : std::optional<std::wstring> {found->second};
    }

    bool application_is_configured(const std::string_view application) {
        const auto snapshot = local_snapshot.load();
        if (!snapshot) return false;
        return snapshot->applications.contains(
            canonical_application_key(std::string {application})
        );
    }

    bool configured_multi_window_cycle(const std::string_view application) {
        const auto snapshot = local_snapshot.load();
        if (!snapshot) return false;
        const auto found = snapshot->applications.find(
            canonical_application_key(std::string {application})
        );
        return found != snapshot->applications.end() &&
            found->second.multi_window == MultiWindowBehavior::cycle;
    }

    std::vector<WindowHandle> matching_windows(
        const std::string_view application
    ) {
        const auto key = canonical_application_key(std::string {application});
        const auto snapshot = local_snapshot.load();
        WindowMatcher matcher;
        if (snapshot) {
            const auto found = snapshot->applications.find(key);
            if (found != snapshot->applications.end()) {
                matcher = found->second.window_matcher;
            }
        }
        if (!matcher.configured()) {
            matcher = compiled_default_matcher(key);
        }
        const auto windows = matching_window_list(matcher);
        std::vector<WindowHandle> result;
        result.reserve(windows.size());
        for (const HWND window : windows) {
            result.push_back(window);
        }
        return result;
    }

    std::optional<NativeActivationDecision> prepare_native_activation(
        const std::string_view application
    ) {
        const auto snapshot = local_snapshot.load();
        if (!snapshot || snapshot->source == SnapshotSource::disabled) {
            return std::nullopt;
        }
        const auto found = snapshot->applications.find(
            canonical_application_key(std::string {application})
        );
        if (found == snapshot->applications.end()) return std::nullopt;
        if (!found->second.position || !found->second.position->valid()) {
            return std::nullopt;
        }

        const ApplicationRoute& route = found->second;
        const std::uint8_t count =
            route.multi_window == MultiWindowBehavior::cycle
                ? matching_window_count(route.window_matcher)
                : 0;
        return NativeActivationDecision {
            .position = *route.position,
            .matching_window_count = count,
            .should_cycle = count >= 2
        };
    }

    bool try_activate_native(const std::string_view application) {
        const auto position = get_native_taskbar_position(application);
        return position && activate_native_position(*position);
    }

    bool activate_native_position(const Position position) {
        return position.valid() &&
            ac::keyboard::send_taskbar_position(position.value);
    }

    bool begin_native_cycle(const Position position) {
        if (!position.valid()) return false;
        ac::keyboard::press_and_hold_winkey();
        ac::keyboard::send_taskbar_position_while_win_held(position.value);
        return true;
    }

    bool advance_native_cycle(const Position position) {
        if (!position.valid()) return false;
        ac::keyboard::send_taskbar_position_while_win_held(position.value);
        return true;
    }

    void end_native_cycle() noexcept {
        ac::keyboard::release_winkey();
    }

    SnapshotInfo snapshot_info() noexcept {
        const auto snapshot = local_snapshot.load();
        if (!snapshot) return {};
        return SnapshotInfo {
            .schema_version = snapshot->schema_version,
            .generation = snapshot->generation,
            .source = snapshot->source,
            .slot_count = snapshot->slots.size(),
            .application_count = snapshot->applications.size(),
            .configured_command_count = snapshot->activation_commands.size()
        };
    }

    std::vector<TaskbarSlotInfo> first_ten_taskbar_slots() {
        const auto snapshot = local_snapshot.load();
        if (!snapshot || snapshot->source == SnapshotSource::disabled) {
            return {};
        }

        std::vector<TaskbarSlotInfo> result;
        result.reserve(snapshot->slots.size());
        for (const Slot& slot : snapshot->slots) {
            TaskbarSlotInfo info {
                .position = slot.position,
                .automation_id = slot.automation_id,
                .application_id = slot.application_id,
                .display_name = slot.diagnostic_display_name
            };
            for (const auto& [application, route] : snapshot->applications) {
                if (route.position && *route.position == slot.position) {
                    info.applications.push_back(application);
                }
            }
            std::ranges::sort(info.applications);
            result.push_back(std::move(info));
        }
        std::ranges::sort(result, {}, [](const TaskbarSlotInfo& slot) {
            return slot.position.value;
        });
        return result;
    }

    std::expected<std::vector<DiscoveredTaskbarApplication>, std::string>
    discover_taskbar_applications() {
        return calculate_all_taskbar_applications();
    }
}
