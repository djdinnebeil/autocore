/**
 * \file main.cxx
 * \brief Session authority for native taskbar snapshots.
 */
import std;
import auto_core.core.component;
import auto_core.core.config;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.core.pipes;
import auto_core.taskbar;
import command_registry;
import taskbar_commands;
import taskbar_config_protocol;
import taskbar_logging;
import taskbar_protocol;
import component_protocol;

import <Windows.h>;

namespace {
    constexpr std::wstring_view authority_mutex_name =
        L"Local\\AutoCore.Taskbar.Authority.v1";

    struct AuthorityGuard {
        ~AuthorityGuard() {
            ac::taskbar::stop_authority();
        }
    };

    class AuthorityMutex {
    public:
        AuthorityMutex() = default;
        AuthorityMutex(const AuthorityMutex&) = delete;
        AuthorityMutex& operator=(const AuthorityMutex&) = delete;

        ~AuthorityMutex() {
            if (acquired_) ReleaseMutex(handle_);
            if (handle_ != nullptr) CloseHandle(handle_);
        }

        [[nodiscard]] bool acquire() {
            handle_ = CreateMutexW(
                nullptr, FALSE, authority_mutex_name.data()
            );
            if (handle_ == nullptr) return false;
            const DWORD result = WaitForSingleObject(handle_, 5000);
            acquired_ = result == WAIT_OBJECT_0 || result == WAIT_ABANDONED;
            return acquired_;
        }

    private:
        HANDLE handle_ {};
        bool acquired_ {};
    };

    class ConfigDiscoveryServer {
    public:
        ConfigDiscoveryServer() = default;
        ConfigDiscoveryServer(const ConfigDiscoveryServer&) = delete;
        ConfigDiscoveryServer& operator=(const ConfigDiscoveryServer&) = delete;

        ~ConfigDiscoveryServer() {
            stop();
        }

        void start() {
            bool expected = false;
            if (!running_.compare_exchange_strong(expected, true)) return;
            thread_ = std::thread([this] { run(); });
        }

        void stop() noexcept {
            if (!running_.exchange(false)) return;
            {
                const std::scoped_lock lock {pipe_mutex_};
                if (active_pipe_ != INVALID_HANDLE_VALUE) {
                    (void)CancelIoEx(active_pipe_, nullptr);
                }
            }
            if (thread_.joinable()) thread_.join();
        }

    private:
        void send_discovery(ac::pipes::Pipe& pipe) {
            const auto discovered =
                ac::taskbar::discover_taskbar_applications();
            if (!discovered) {
                (void)ac::pipes::send_string(
                    pipe, std::string {"error:"} + discovered.error()
                );
                return;
            }

            if (!ac::pipes::send_string(
                    pipe, ac::protocol::taskbar_config::success) ||
                !ac::pipes::send_string(
                    pipe, std::to_string(discovered->size()))) {
                return;
            }

            for (const auto& application : *discovered) {
                if (!ac::pipes::send_string(
                        pipe, std::to_string(application.ordinal)) ||
                    !ac::pipes::send_string(pipe, application.display_name) ||
                    !ac::pipes::send_string(pipe, application.automation_id) ||
                    !ac::pipes::send_string(pipe, application.application_id)) {
                    return;
                }
            }
        }

        void serve(ac::pipes::Pipe& pipe) {
            ac::pipes::CommandDispatcher dispatcher;
            dispatcher.set_command(
                ac::protocol::taskbar_config::to_wire(
                    ac::protocol::taskbar_config::Request::discover_all
                ),
                [&] {
                    send_discovery(pipe);
                    dispatcher.request_stop();
                }
            );
            (void)dispatcher.process(pipe);
        }

        void run() {
            while (running_.load()) {
                auto created = ac::pipes::create_pipe_server(
                    std::wstring {ac::protocol::taskbar_config::pipe_name}
                );
                if (!created) return;
                ac::pipes::Pipe pipe = std::move(*created);
                const HANDLE handle = static_cast<HANDLE>(
                    pipe.native_handle()
                );
                {
                    const std::scoped_lock lock {pipe_mutex_};
                    active_pipe_ = handle;
                }

                const BOOL connected = ConnectNamedPipe(handle, nullptr);
                const DWORD connect_error = connected
                    ? ERROR_SUCCESS
                    : GetLastError();
                if (running_.load() &&
                    (connected || connect_error == ERROR_PIPE_CONNECTED)) {
                    serve(pipe);
                    // DisconnectNamedPipe discards unread buffered data. Wait
                    // until the configuration client has consumed the full
                    // discovery response before closing this pipe instance.
                    (void)FlushFileBuffers(handle);
                }
                (void)DisconnectNamedPipe(handle);
                {
                    const std::scoped_lock lock {pipe_mutex_};
                    if (active_pipe_ == handle) {
                        active_pipe_ = INVALID_HANDLE_VALUE;
                    }
                }
            }
        }

        std::atomic_bool running_ {false};
        std::mutex pipe_mutex_;
        HANDLE active_pipe_ {INVALID_HANDLE_VALUE};
        std::thread thread_;
    };

    int write_manifest(
        const command_registry::Registry& registry,
        const std::filesystem::path& destination
    ) {
        std::set<std::string> commands;
        for (const std::string& value : registry.autocomplete_values()) {
            commands.insert(value);
        }

        const std::filesystem::path applications =
            ac::paths::taskbar_directory();
        std::error_code error;
        if (std::filesystem::is_directory(applications, error)) {
            for (std::filesystem::directory_iterator iterator(
                     applications, error
                 );
                 !error && iterator !=
                     std::filesystem::directory_iterator {};
                 iterator.increment(error)) {
                if (!iterator->is_regular_file(error) ||
                    iterator->path().extension() != ".ini") {
                    continue;
                }

                std::ifstream input(iterator->path());
                std::string section;
                std::string line;
                while (std::getline(input, line)) {
                    const auto first = line.find_first_not_of(" \t\r");
                    if (first == std::string::npos ||
                        line[first] == ';' || line[first] == '#') {
                        continue;
                    }
                    const auto last = line.find_last_not_of(" \t\r");
                    const std::string_view value {line.data() + first,
                                                  last - first + 1};
                    if (value.starts_with('[') && value.ends_with(']')) {
                        section = value.substr(1, value.size() - 2);
                        std::ranges::transform(
                            section, section.begin(), [](unsigned char ch) {
                                return static_cast<char>(std::tolower(ch));
                            }
                        );
                        continue;
                    }
                    if (section != "commands") continue;
                    const auto equals = value.find('=');
                    if (equals == std::string_view::npos) continue;
                    std::string key {value.substr(0, equals)};
                    while (!key.empty() &&
                           std::isspace(static_cast<unsigned char>(key.back()))) {
                        key.pop_back();
                    }
                    std::ranges::transform(
                        key, key.begin(), [](unsigned char ch) {
                            return static_cast<char>(std::tolower(ch));
                        }
                    );
                    if (key != "activate") continue;

                    std::string_view aliases = value.substr(equals + 1);
                    std::size_t alias_first {};
                    while (alias_first <= aliases.size()) {
                        const auto separator = aliases.find('|', alias_first);
                        std::string alias {aliases.substr(
                            alias_first,
                            separator == std::string_view::npos
                                ? std::string_view::npos
                                : separator - alias_first
                        )};
                        const auto alias_begin =
                            alias.find_first_not_of(" \t\r");
                        if (alias_begin != std::string::npos) {
                            const auto alias_end =
                                alias.find_last_not_of(" \t\r");
                            commands.insert(alias.substr(
                                alias_begin, alias_end - alias_begin + 1
                            ));
                        }
                        if (separator == std::string_view::npos) break;
                        alias_first = separator + 1;
                    }
                }
            }
        }

        if (commands.contains("activate_file_explorer")) {
            commands.insert("activate_folder");
        }
        if (commands.contains("activate_google_chrome")) {
            commands.insert("activate_chrome");
        }
        if (commands.contains("activate_visual_studio")) {
            commands.insert("activate_visual");
        }
        if (commands.contains("activate_visual_studio_code")) {
            commands.insert("activate_vs_code");
        }
        if (commands.contains("activate_zoom_workplace")) {
            commands.insert("activate_zoom");
        }

        std::filesystem::path temporary = destination;
        temporary += ".tmp";
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) return 1;

        for (const std::string& value : commands) {
            output << value << '\n';
        }
        output.close();
        if (!output) return 1;

        return MoveFileExW(
            temporary.c_str(),
            destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        ) ? 0 : 1;
    }

    std::string joined_applications(
        const std::vector<std::string>& applications
    ) {
        std::string result;
        for (const std::string& application : applications) {
            if (!result.empty()) result += ", ";
            result += application;
        }
        return result;
    }

    void report_taskbar_mappings(const ac::taskbar::SnapshotInfo& snapshot) {
        if (ac::config::core_settings().warn_without_winkey_mapping &&
            !ac::taskbar::get_native_taskbar_position("auto_core")) {
            taskbar_component().log_and_print(
                "Performance warning: Auto Core is not mapped to taskbar "
                "positions 1 through 10. Auto Core will use direct console "
                "window activation as a fallback, which can occasionally "
                "have higher latency. Set "
                "warn_without_winkey_mapping = false under [main] in "
                "config/main.ini to silence this warning."
            );
        }

        if (snapshot.source == ac::taskbar::SnapshotSource::disabled) {
            taskbar_component().log_and_print(
                "Taskbar mapping is disabled; no Winkey mappings were set."
            );
            return;
        }
        if (snapshot.source == ac::taskbar::SnapshotSource::configured) {
            bool cached = false;
            for (const auto& command :
                 ac::taskbar::configured_activation_commands()) {
                if (ac::taskbar::get_native_taskbar_position(
                        command.application
                    )) {
                    cached = true;
                    break;
                }
            }
            if (cached) {
                taskbar_component().log_and_log(
                    "Using cached taskbar positions from "
                    "taskbar/cached_positions.ini. Native Win+N mappings were not "
                    "verified against the live taskbar."
                );
            }
            else {
                taskbar_component().log_and_print(
                    "Live taskbar discovery was unavailable; native Win+N "
                    "mappings were not set."
                );
            }
            return;
        }

        for (const auto& slot : ac::taskbar::first_ten_taskbar_slots()) {
            const std::string shortcut = slot.position.value == 10
                ? "Win+0"
                : std::format("Win+{}", slot.position.value);
            const std::string_view display_name = slot.display_name.empty()
                ? std::string_view {"<unnamed>"}
                : std::string_view {slot.display_name};

            if (slot.applications.empty()) {
                taskbar_component().log_and_print(
                    "Taskbar position {} ({}): '{}' with application ID '{}' "
                    "has no matching application configuration; Winkey "
                    "mapping not set.",
                    slot.position.value,
                    shortcut,
                    display_name,
                    slot.application_id
                );
            }
            else {
                taskbar_component().log_and_log(
                    "Taskbar position {} ({}): '{}' with application ID '{}' "
                    "-> key = {}.",
                    slot.position.value,
                    shortcut,
                    display_name,
                    slot.application_id,
                    joined_applications(slot.applications)
                );
            }
        }
    }
}

int main(const int argument_count, char* arguments[]) {
    ac::config::initialize_core_settings();
    if (!ac::config::core_settings_report().empty()) {
        taskbar_component().log_and_print(
            "{}",
            ac::config::core_settings_report()
        );
    }
    auto registry = create_taskbar_command_registry();
    std::optional<DWORD> standalone_parent;
    if (argument_count == 3 &&
        std::string_view {arguments[1]} ==
            "--standalone-config-authority") {
        unsigned long parsed {};
        const std::string_view value {arguments[2]};
        const auto converted = std::from_chars(
            value.data(), value.data() + value.size(), parsed
        );
        if (converted.ec != std::errc {} ||
            converted.ptr != value.data() + value.size() || parsed == 0) {
            return 1;
        }
        standalone_parent = static_cast<DWORD>(parsed);
    }
    if (argument_count == 3 &&
        std::string_view {arguments[1]} ==
            "--generate-keymap-command-registry") {
        return write_manifest(registry, arguments[2]);
    }
    if (argument_count == 3 &&
        std::string_view {arguments[1]} == "--inspect-snapshot") {
        if (!ac::taskbar::connect(std::chrono::seconds {5})) return 1;
        struct ClientGuard {
            ~ClientGuard() { ac::taskbar::disconnect(); }
        } client_guard;

        const std::string_view application {arguments[2]};
        const auto decision =
            ac::taskbar::prepare_native_activation(application);
        const auto windows = ac::taskbar::matching_windows(application);
        const bool compiled_default =
            application == "wordpad" || application == "powershell_admin";
        if (decision) {
            std::cout << std::format(
                "application={} position={} matching_windows={} cycle={}\n",
                application,
                decision->position.value,
                decision->matching_window_count,
                decision->should_cycle
            );
        }
        else if (
            ac::taskbar::application_is_configured(application) ||
            compiled_default
        ) {
            const bool cycle =
                ac::taskbar::configured_multi_window_cycle(application) &&
                windows.size() >= 2;
            std::cout << std::format(
                "application={} position=emulated matching_windows={} "
                "cycle={}\n",
                application,
                windows.size(),
                cycle
            );
        }
        else {
            std::cout << std::format(
                "application={} unavailable\n", application
            );
            return 2;
        }
        for (const auto& command :
             ac::taskbar::configured_activation_commands()) {
            if (command.application == application) {
                std::cout << std::format(
                    "command={} application={}\n",
                    command.command,
                    command.application
                );
            }
        }
        return 0;
    }

    taskbar_component().connect_to_logger();
    taskbar_component().log_and_log("taskbar_ac.exe started");

    {
        const auto ini_path = ac::paths::config_directory() / "taskbar.ini";
        if (!ac::ini::read(ini_path)) {
            std::error_code exists_error;
            const bool present =
                std::filesystem::exists(ini_path, exists_error);
            taskbar_component().report_ini_unavailable(
                present && !exists_error
            );
        }
    }

    AuthorityMutex authority_mutex;
    if (!authority_mutex.acquire()) {
        taskbar_component().log_and_print(
            "Another taskbar snapshot authority is still active."
        );
        return 1;
    }

    if (!ac::taskbar::start_authority()) {
        taskbar_component().log_and_print(
            "Unable to start the native taskbar snapshot authority."
        );
        return 1;
    }
    AuthorityGuard authority_guard;

    if (standalone_parent) {
        if (!ac::taskbar::wait_for_initial_snapshot(
                std::chrono::seconds {5})) {
            taskbar_component().log_and_print(
                "Timed out waiting for the standalone taskbar snapshot."
            );
            return 1;
        }

        ConfigDiscoveryServer config_server;
        config_server.start();
        taskbar_component().log_and_log(
            "Standalone taskbar configuration authority is ready."
        );

        const HANDLE parent = OpenProcess(
            SYNCHRONIZE, FALSE, *standalone_parent
        );
        if (parent == nullptr) {
            taskbar_component().log_and_print(
                "Unable to monitor taskbar_config.exe. Error: {}",
                GetLastError()
            );
            return 1;
        }
        (void)WaitForSingleObject(parent, INFINITE);
        CloseHandle(parent);
        return 0;
    }

    auto connection = ac::pipes::connect_to_pipe_server(
        ac::protocol::component::pipe_name("taskbar")
    );
    if (!connection) {
        taskbar_component().log_and_print(
            "Failed to connect to the taskbar control pipe. Error: {}",
            connection.error().system_error
        );
        return 1;
    }
    ac::pipes::Pipe control_pipe = std::move(*connection);

    if (!ac::taskbar::wait_for_initial_snapshot(std::chrono::seconds {5})) {
        taskbar_component().log_and_print(
            "Timed out waiting for the initial native taskbar snapshot."
        );
        return 1;
    }

    const auto snapshot = ac::taskbar::snapshot_info();
    taskbar_component().log_and_log(
        "Published taskbar snapshot generation {} with {} slots, {} "
        "application routes, and {} configured commands.",
        snapshot.generation,
        snapshot.slot_count,
        snapshot.application_count,
        snapshot.configured_command_count
    );
    report_taskbar_mappings(snapshot);

    ConfigDiscoveryServer config_server;
    config_server.start();

    if (const auto ready = ac::pipes::send_string(
            control_pipe,
            ac::protocol::component::make_hello(registry.autocomplete_values())
        ); !ready) {
        taskbar_component().log_and_print(
            "Failed to signal taskbar readiness. Error: {}",
            ready.error().system_error
        );
        return 1;
    }

    ac::pipes::CommandDispatcher dispatcher;
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::invoke
        ),
        [&control_pipe, &registry, &dispatcher] {
            const auto name = ac::pipes::read_string(control_pipe);
            if (!name) {
                taskbar_component().log_and_print(
                    "Failed to read a taskbar command. Error: {}",
                    name.error().system_error
                );
                dispatcher.request_stop();
                return;
            }
            auto action = registry.resolve(*name);
            if (!action) {
                taskbar_component().log_and_print(
                    "Unknown taskbar command: {}", *name
                );
                return;
            }
            action();
        }
    );
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::shutdown
        ),
        [&dispatcher] {
            taskbar_component().log_and_log("shutdown signal received");
            dispatcher.request_stop();
        }
    );

    if (const auto result = dispatcher.process(control_pipe); !result) {
        taskbar_component().log_and_print(
            "Taskbar control pipe ended. Error: {}",
            result.error().system_error
        );
        return 1;
    }

    taskbar_component().log_and_log("program terminated");
    return 0;
}
