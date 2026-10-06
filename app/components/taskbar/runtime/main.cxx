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
import taskbar_logging;
import taskbar_protocol;
import component_protocol;

import <Windows.h>;
import auto_core.core.shell;

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
            taskbar_component().log_print(
                "Performance warning: Auto Core is not mapped to taskbar "
                "positions 1 through 10. Auto Core will use direct console "
                "window activation as a fallback, which can occasionally "
                "have higher latency. Set "
                "warn_without_winkey_mapping = off under [auto_core] in "
                "config/auto_core.ini to silence this warning."
            );
        }

        if (snapshot.source == ac::taskbar::SnapshotSource::disabled) {
            taskbar_component().log_print(
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
                taskbar_component().log_main(
                    "Using cached taskbar positions from "
                    "taskbar/winkey_map.cache. Native Win+N mappings were not "
                    "verified against the live taskbar."
                );
            }
            else {
                taskbar_component().log_print(
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
                taskbar_component().log_print(
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
                taskbar_component().log_main(
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
    ac::shell::set_process_app_user_model_id();
    ac::config::initialize_core_settings();
    if (!ac::config::core_settings_report().empty()) {
        taskbar_component().log_print(
            "{}",
            ac::config::core_settings_report()
        );
    }
    auto registry = create_taskbar_command_registry();
    if (argument_count == 2 &&
        std::string_view {arguments[1]} == "--refresh-cache") {
        const auto ini_path = ac::paths::config_directory() / "taskbar.ini";
        if (!ac::ini::read(ini_path)) {
            std::error_code exists_error;
            const bool present =
                std::filesystem::exists(ini_path, exists_error);
            taskbar_component().report_ini_unavailable(
                present && !exists_error
            );
        }
        if (!ac::taskbar::refresh_winkey_cache()) {
            taskbar_component().log_print(
                "Unable to refresh winkey_map.cache."
            );
            return 1;
        }
        taskbar_component().log_main("Refreshed winkey_map.cache.");
        return 0;
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

    taskbar_component().log_main("taskbar_ac.exe started");

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
        taskbar_component().log_print(
            "Another taskbar snapshot authority is still active."
        );
        return 1;
    }

    if (!ac::taskbar::start_authority()) {
        taskbar_component().log_print(
            "Unable to start the native taskbar snapshot authority."
        );
        return 1;
    }
    AuthorityGuard authority_guard;


    auto connection = ac::pipes::connect_to_pipe_server(
        ac::protocol::component::pipe_name("taskbar")
    );
    if (!connection) {
        taskbar_component().log_print(
            "Failed to connect to the taskbar control pipe. Error: {}",
            connection.error().system_error
        );
        return 1;
    }
    ac::pipes::Pipe control_pipe = std::move(*connection);

    if (!ac::taskbar::wait_for_initial_snapshot(std::chrono::seconds {5})) {
        taskbar_component().log_print(
            "Timed out waiting for the initial native taskbar snapshot."
        );
        return 1;
    }

    const auto snapshot = ac::taskbar::snapshot_info();
    taskbar_component().log_main(
        "Published taskbar snapshot generation {} with {} slots, {} "
        "application routes, and {} configured commands.",
        snapshot.generation,
        snapshot.slot_count,
        snapshot.application_count,
        snapshot.configured_command_count
    );
    report_taskbar_mappings(snapshot);


    if (const auto ready = ac::pipes::send_string(
            control_pipe,
            ac::protocol::component::make_hello(registry.autocomplete_values())
        ); !ready) {
        taskbar_component().log_print(
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
                taskbar_component().log_print(
                    "Failed to read a taskbar command. Error: {}",
                    name.error().system_error
                );
                dispatcher.request_stop();
                return;
            }
            auto action = registry.resolve(*name);
            if (!action) {
                taskbar_component().log_print(
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
            taskbar_component().log_main("shutdown signal received");
            dispatcher.request_stop();
        }
    );

    if (const auto result = dispatcher.process(control_pipe); !result) {
        taskbar_component().log_print(
            "Taskbar control pipe ended. Error: {}",
            result.error().system_error
        );
        return 1;
    }

    taskbar_component().log_main("program terminated");
    return 0;
}
