/**
 * \file ac_components.cxx
 * \brief Generic host: open `components.list` `[components]`, hello, invoke, shutdown.
 */
module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include "../../../core/src/components_list_detail.hpp"
#include "logger_shutdown_detail.hpp"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker, \
    "/manifestdependency:\"type='win32' " \
    "name='Microsoft.Windows.Common-Controls' " \
    "version='6.0.0.0' processorArchitecture='*' " \
    "publicKeyToken='6595b64144ccf1df' language='*'\"")

module auto_core.main.components;

import std;
import auto_core.core.ini;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.core.pipes;
import auto_core.main.application;
import auto_core.taskbar;
import component_protocol;

namespace {

    constexpr auto hello_timeout = std::chrono::seconds {5};
    enum class DelayedShutdownPrompt {
        popup,
        console,
    };

    struct ShutdownSettings {
        DelayedShutdownPrompt prompt {DelayedShutdownPrompt::popup};
        std::chrono::milliseconds timeout {2000};
    };

    enum class RecoveryChoice {
        force_eligible,
        leave_running,
        all_exited,
        continue_waiting,
    };

    constexpr int force_terminate_button = 1001;
    constexpr int leave_running_button = 1002;

    struct StartedChild {
        std::string name;
        ac::pipes::Pipe pipe;
        std::mutex pipe_mutex;
        HANDLE process {nullptr};
        ac::protocol::component::TerminationPolicy termination_policy {
            ac::protocol::component::TerminationPolicy::graceful
        };
        std::vector<ac::protocol::component::CatalogEntry> catalog;
        bool snapshot_attached {false};
    };

    struct Launch {
        std::string name;
        ac::pipes::Pipe pipe;
        HANDLE process {nullptr};
    };

    std::vector<std::unique_ptr<StartedChild>> started_children;
    bool shutdown_complete {true};
    ac::config::components_list::ParseResult catalog;
    bool catalog_loaded {false};

    [[nodiscard]]
    const ac::config::components_list::ParseResult& loaded_catalog() {
        if (!catalog_loaded) {
            const auto list_path = ac::paths::components_list_file();
            const auto runtime = ac::config::components_list::load_runtime_catalog(
                list_path,
                ac::paths::bin_directory()
            );
            if (runtime.used_discovery) {
                std::error_code exists_error;
                const bool present =
                    std::filesystem::exists(list_path, exists_error);
                if (present && !exists_error) {
                    auto_core.log_print(
                        "components.list is unreadable. Run "
                        "components_editor.exe to generate a valid file. "
                        "Discovering *_ac.exe in the binary directory; the "
                        "file will not be created."
                    );
                }
                else {
                    auto_core.log_print(
                        "components.list is missing. Run "
                        "components_editor.exe to generate it. Discovering "
                        "*_ac.exe in the binary directory; the file will not "
                        "be created."
                    );
                }
            }
            catalog = runtime.result;
            catalog_loaded = true;
        }
        return catalog;
    }

    [[nodiscard]]
    ShutdownSettings load_shutdown_settings() {
        ShutdownSettings settings;
        const auto path = ac::paths::config_directory() / "shutdown.ini";
        const auto document = ac::ini::read(path);
        if (!document) {
            std::error_code exists_error;
            const bool present = std::filesystem::exists(path, exists_error);
            if (present && !exists_error) {
                auto_core.log_print(
                    "config/shutdown.ini is malformed. Run shutdown_config.exe "
                    "to generate a valid file. Using built-in defaults; the "
                    "file will not be created."
                );
            }
            else {
                auto_core.log_print(
                    "config/shutdown.ini is missing. Run shutdown_config.exe "
                    "to generate it. Using built-in defaults; the file will "
                    "not be created."
                );
            }
            return settings;
        }

        const auto prompt = document->find(
            "shutdown", "delayed_shutdown_prompt"
        );
        const auto timeout = document->find(
            "shutdown", "shutdown_timeout_ms"
        );
        if (!prompt || !timeout) {
            auto_core.log_print(
                "config/shutdown.ini is missing an expected [shutdown] key. "
                "Run shutdown_config.exe to repair it. Using built-in "
                "defaults for missing keys; the file will not be rewritten."
            );
        }

        if (prompt && *prompt == "console") {
            settings.prompt = DelayedShutdownPrompt::console;
        }
        else if (prompt && *prompt != "popup") {
            auto_core.log_print(
                "config/shutdown.ini [shutdown] delayed_shutdown_prompt is "
                "invalid. Run shutdown_config.exe to repair it. Using popup; "
                "the file will not be rewritten."
            );
        }

        if (timeout) {
            std::uint64_t milliseconds = 0;
            const auto parsed = std::from_chars(
                timeout->data(),
                timeout->data() + timeout->size(),
                milliseconds
            );
            if (parsed.ec == std::errc {} &&
                parsed.ptr == timeout->data() + timeout->size() &&
                milliseconds <= (std::numeric_limits<DWORD>::max)()) {
                settings.timeout = std::chrono::milliseconds {milliseconds};
            }
            else {
                auto_core.log_print(
                    "config/shutdown.ini [shutdown] shutdown_timeout_ms is "
                    "invalid. Run shutdown_config.exe to repair it. Using "
                    "2000; the file will not be rewritten."
                );
            }
        }
        return settings;
    }

    [[nodiscard]]
    const ShutdownSettings& shutdown_settings() {
        static const ShutdownSettings settings = load_shutdown_settings();
        return settings;
    }

    StartedChild* find_child(const std::string_view name) {
        for (auto& child : started_children) {
            if (child && child->name == name) {
                return child.get();
            }
        }
        return nullptr;
    }

    void close_process_handle(HANDLE& process) noexcept {
        if (process != nullptr && process != INVALID_HANDLE_VALUE) {
            CloseHandle(process);
        }
        process = nullptr;
    }

    void terminate_and_close(HANDLE& process) noexcept {
        if (process == nullptr || process == INVALID_HANDLE_VALUE) {
            process = nullptr;
            return;
        }
        (void)TerminateProcess(process, 1);
        (void)WaitForSingleObject(process, 2000);
        close_process_handle(process);
    }

    void abandon_child(ac::pipes::Pipe& pipe, HANDLE& process) noexcept {
        (void)pipe.cancel();
        pipe.reset();
        terminate_and_close(process);
    }

    [[nodiscard]]
    HANDLE start_owned_process(const std::filesystem::path& executable_path) {
        STARTUPINFOW startup_info {};
        startup_info.cb = sizeof(startup_info);
        PROCESS_INFORMATION process_info {};

        std::wstring command_line = L"\"" + executable_path.wstring() + L"\"";
        if (!CreateProcessW(
                executable_path.c_str(),
                command_line.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                executable_path.parent_path().c_str(),
                &startup_info,
                &process_info
            )) {
            auto_core.print(
                "Unable to start '{}'. GetLastError = {}",
                executable_path,
                GetLastError()
            );
            return nullptr;
        }

        (void)AllowSetForegroundWindow(process_info.dwProcessId);
        CloseHandle(process_info.hThread);
        return process_info.hProcess;
    }

    bool launch_detached_logger_once(
        const std::filesystem::path& executable
    ) {
        std::wstring command_line =
            L"\"" + executable.wstring() + L"\" --once --shutdown";
        const auto directory = executable.parent_path();
        const auto try_create = [&](const DWORD flags) {
            STARTUPINFOW startup_info {};
            startup_info.cb = sizeof(startup_info);
            PROCESS_INFORMATION process_info {};
            if (!CreateProcessW(
                    executable.c_str(),
                    command_line.data(),
                    nullptr,
                    nullptr,
                    FALSE,
                    flags,
                    nullptr,
                    directory.c_str(),
                    &startup_info,
                    &process_info
                )) {
                return false;
            }
            CloseHandle(process_info.hThread);
            CloseHandle(process_info.hProcess);
            return true;
        };

        const DWORD breakaway = DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP |
            CREATE_BREAKAWAY_FROM_JOB;
        if (try_create(breakaway)) {
            return true;
        }
        return try_create(DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP);
    }

    [[nodiscard]]
    std::optional<Launch> try_launch(const std::string& name) {
        namespace protocol = ac::protocol::component;

        auto pipe_result = ac::pipes::create_pipe_server(protocol::pipe_name(name));
        if (!pipe_result) {
            auto_core.log_print(
                "Failed to create the {} control pipe. Error: {}",
                name,
                pipe_result.error().system_error
            );
            return std::nullopt;
        }

        const auto executable =
            ac::paths::bin_directory() / protocol::executable_name(name);
        std::error_code exists_error;
        if (!std::filesystem::exists(executable, exists_error)) {
            auto_core.log_print(
                "{} is missing; {} is unavailable for this session.",
                executable.filename(),
                name
            );
            pipe_result->reset();
            return std::nullopt;
        }

        HANDLE process = start_owned_process(executable);
        if (process == nullptr) {
            auto_core.log_print(
                "Unable to start {}; {} is unavailable for this session.",
                executable.filename(),
                name
            );
            pipe_result->reset();
            return std::nullopt;
        }

        return Launch {name, std::move(*pipe_result), process};
    }

    [[nodiscard]]
    bool child_has_exited(const HANDLE process) noexcept {
        return process != nullptr &&
            process != INVALID_HANDLE_VALUE &&
            WaitForSingleObject(process, 0) == WAIT_OBJECT_0;
    }

    [[nodiscard]]
    ac::pipes::Result<std::string> wait_for_hello(
        ac::pipes::Pipe& pipe,
        const HANDLE process
    ) {
        using HelloResult = ac::pipes::Result<std::string>;
        std::promise<HelloResult> result_promise;
        std::future<HelloResult> result = result_promise.get_future();
        const auto deadline = std::chrono::steady_clock::now() + hello_timeout;

        std::jthread reader([&result_promise, &pipe, process, deadline] {
            constexpr auto retry_interval = std::chrono::milliseconds {10};

            while (true) {
                if (child_has_exited(process)) {
                    result_promise.set_value(
                        std::unexpected(
                            ac::pipes::Error {ERROR_PROCESS_ABORTED}
                        )
                    );
                    return;
                }

                auto message = ac::pipes::read_string(pipe);
                if (message ||
                    (message.error().system_error != ERROR_PIPE_LISTENING &&
                     message.error().system_error != ERROR_PIPE_NOT_CONNECTED) ||
                    std::chrono::steady_clock::now() >= deadline) {
                    result_promise.set_value(std::move(message));
                    return;
                }
                std::this_thread::sleep_for(retry_interval);
            }
        });

        while (result.wait_for(std::chrono::milliseconds {10}) !=
            std::future_status::ready) {
            if (child_has_exited(process) ||
                std::chrono::steady_clock::now() >= deadline) {
                (void)pipe.cancel();
                break;
            }
        }

        if (reader.joinable()) {
            reader.join();
        }

        if (result.wait_for(std::chrono::milliseconds {0}) ==
            std::future_status::ready) {
            return result.get();
        }
        if (child_has_exited(process)) {
            return std::unexpected(ac::pipes::Error {ERROR_PROCESS_ABORTED});
        }
        return std::unexpected(ac::pipes::Error {WAIT_TIMEOUT});
    }

    struct HelloInFlight {
        Launch launch;
        std::promise<ac::pipes::Result<std::string>> promise;
        std::future<ac::pipes::Result<std::string>> future;
        std::jthread reader;
    };

    void start_hello_reader(HelloInFlight& state) {
        state.future = state.promise.get_future();
        auto& promise = state.promise;
        auto& pipe = state.launch.pipe;
        const HANDLE process = state.launch.process;
        state.reader = std::jthread([&promise, &pipe, process] {
            constexpr auto retry_interval = std::chrono::milliseconds {10};
            const auto deadline = std::chrono::steady_clock::now() + hello_timeout;

            while (true) {
                if (child_has_exited(process)) {
                    promise.set_value(
                        std::unexpected(
                            ac::pipes::Error {ERROR_PROCESS_ABORTED}
                        )
                    );
                    return;
                }

                auto message = ac::pipes::read_string(pipe);
                if (message ||
                    (message.error().system_error != ERROR_PIPE_LISTENING &&
                     message.error().system_error != ERROR_PIPE_NOT_CONNECTED) ||
                    std::chrono::steady_clock::now() >= deadline) {
                    promise.set_value(std::move(message));
                    return;
                }
                std::this_thread::sleep_for(retry_interval);
            }
        });
    }

    [[nodiscard]]
    ac::pipes::Result<std::string> finish_hello_reader(
        HelloInFlight& state,
        const std::chrono::steady_clock::time_point deadline
    ) {
        ac::pipes::Result<std::string> hello_bytes;
        while (state.future.wait_for(std::chrono::milliseconds {10}) !=
            std::future_status::ready) {
            if (child_has_exited(state.launch.process) ||
                std::chrono::steady_clock::now() >= deadline) {
                (void)state.launch.pipe.cancel();
                break;
            }
        }

        if (state.reader.joinable()) {
            state.reader.join();
        }

        if (state.future.wait_for(std::chrono::milliseconds {0}) ==
            std::future_status::ready) {
            hello_bytes = state.future.get();
        }
        else if (child_has_exited(state.launch.process)) {
            hello_bytes = std::unexpected(
                ac::pipes::Error {ERROR_PROCESS_ABORTED}
            );
        }
        else {
            hello_bytes = std::unexpected(ac::pipes::Error {WAIT_TIMEOUT});
        }

        return hello_bytes;
    }

    [[nodiscard]]
    std::unique_ptr<StartedChild> accept_hello(
        Launch launch,
        const ac::pipes::Result<std::string>& hello_bytes
    ) {
        namespace protocol = ac::protocol::component;
        const auto& name = launch.name;

        if (!hello_bytes) {
            auto_core.log_print(
                "{} did not complete hello. Error: {}",
                name,
                hello_bytes.error().system_error
            );
            abandon_child(launch.pipe, launch.process);
            return nullptr;
        }

        const auto hello = protocol::parse_hello(*hello_bytes);
        if (!hello) {
            if (hello.error() == protocol::HelloError::version_mismatch) {
                auto_core.log_print(
                    "{} hello is not {}: that component is unavailable.",
                    name,
                    protocol::protocol_id
                );
            }
            else {
                auto_core.log_print(
                    "{} sent an empty hello; that component is unavailable.",
                    name
                );
            }
            abandon_child(launch.pipe, launch.process);
            return nullptr;
        }

        for (const auto& line : hello->skipped_lines) {
            auto_core.log_print(
                "{} skipped malformed catalog entry: {}",
                name,
                line
            );
        }

        auto child = std::make_unique<StartedChild>();
        child->name = name;
        child->pipe = std::move(launch.pipe);
        child->process = launch.process;
        child->termination_policy = hello->termination_policy;
        launch.process = nullptr;

        auto_core.log_main(
            "Component contract accepted: name={}, pid={}, "
            "termination_policy={}",
            child->name,
            GetProcessId(child->process),
            protocol::to_string(child->termination_policy)
        );

        std::unordered_set<std::string> seen;
        for (auto& entry : hello->commands) {
            if (!seen.insert(entry.name).second) {
                auto_core.log_print(
                    "{} skipped duplicate catalog name '{}'.",
                    name,
                    entry.name
                );
                continue;
            }
            child->catalog.push_back(std::move(entry));
        }

        return child;
    }

    void keep_child(std::unique_ptr<StartedChild> child) {
        started_children.push_back(std::move(child));
        shutdown_complete = false;
    }

    void attach_taskbar_snapshot(StartedChild& child) {
        if (!ac::taskbar::connect()) {
            auto_core.log_print(
                "taskbar_ac.exe did not publish a snapshot; native "
                "taskbar activation is unavailable for this session."
            );
            return;
        }

        child.snapshot_attached = true;
        const auto snapshot = ac::taskbar::snapshot_info();
        auto_core.log_main(
            "Main installed taskbar snapshot generation {} with {} "
            "slots and {} application routes.",
            snapshot.generation,
            snapshot.slot_count,
            snapshot.application_count
        );
    }

    void start_taskbar() {
        auto launched = try_launch("taskbar");
        if (!launched) {
            return;
        }

        const auto hello_bytes = wait_for_hello(
            launched->pipe,
            launched->process
        );
        auto child = accept_hello(std::move(*launched), hello_bytes);
        if (!child) {
            return;
        }

        attach_taskbar_snapshot(*child);
        keep_child(std::move(child));
    }

    void start_remaining(const std::vector<std::string>& names) {
        std::vector<std::unique_ptr<HelloInFlight>> inflight;
        inflight.reserve(names.size());

        for (const auto& name : names) {
            auto launched = try_launch(name);
            if (!launched) {
                continue;
            }
            auto state = std::make_unique<HelloInFlight>();
            state->launch = std::move(*launched);
            start_hello_reader(*state);
            inflight.push_back(std::move(state));
        }

        const auto deadline = std::chrono::steady_clock::now() + hello_timeout;
        for (auto& state : inflight) {
            const auto hello_bytes = finish_hello_reader(*state, deadline);
            auto child = accept_hello(std::move(state->launch), hello_bytes);
            if (child) {
                keep_child(std::move(child));
            }
        }
    }

    void request_child_shutdown(StartedChild& child) noexcept {
        const std::scoped_lock lock {child.pipe_mutex};
        if (child.pipe.valid()) {
            if (const auto result = ac::pipes::send_pipe_command_nowait(
                    child.pipe,
                    ac::protocol::component::to_wire(
                        ac::protocol::component::Request::shutdown
                    )
                ); !result) {
                auto_core.log_print(
                    "Failed to stop {}. Error: {}",
                    child.name,
                    result.error().system_error
                );
            }
        }
    }

    void disconnect_child_snapshot(StartedChild& child) noexcept {
        if (child.snapshot_attached) {
            ac::taskbar::disconnect();
            child.snapshot_attached = false;
        }
    }

    void close_child_handles(StartedChild& child) noexcept {
        const std::scoped_lock lock {child.pipe_mutex};
        child.pipe.reset();
        close_process_handle(child.process);
    }

    [[nodiscard]]
    std::vector<HANDLE> active_process_handles() {
        std::vector<HANDLE> handles;
        handles.reserve(started_children.size());
        for (const auto& child : started_children) {
            if (child && child->process != nullptr &&
                child->process != INVALID_HANDLE_VALUE) {
                handles.push_back(child->process);
            }
        }
        return handles;
    }

    void wait_for_process_exit(
        const std::vector<HANDLE>& processes,
        const std::chrono::steady_clock::time_point deadline
    ) {
        for (std::size_t first = 0; first < processes.size();
             first += MAXIMUM_WAIT_OBJECTS) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                return;
            }

            const auto remaining =
                std::chrono::ceil<std::chrono::milliseconds>(deadline - now);
            const DWORD timeout = static_cast<DWORD>(remaining.count());
            const DWORD count = static_cast<DWORD>((std::min)(
                processes.size() - first,
                static_cast<std::size_t>(MAXIMUM_WAIT_OBJECTS)
            ));
            const DWORD result = WaitForMultipleObjects(
                count,
                processes.data() + first,
                TRUE,
                timeout
            );

            if (result == WAIT_TIMEOUT) {
                return;
            }
            if (result == WAIT_FAILED) {
                auto_core.log_main(
                    "Unable to wait for component processes. GetLastError = {}",
                    GetLastError()
                );
                return;
            }
        }
    }

    [[nodiscard]]
    std::vector<StartedChild*> lingering_children(
        const bool include_logger
    ) {
        std::vector<StartedChild*> lingering;
        for (auto& child : started_children) {
            if (!child || child->process == nullptr ||
                child->process == INVALID_HANDLE_VALUE) {
                continue;
            }
            if (!include_logger &&
                ac::main::components::detail::is_logger(child->name)) {
                continue;
            }
            if (WaitForSingleObject(child->process, 0) != WAIT_OBJECT_0) {
                lingering.push_back(child.get());
            }
        }
        return lingering;
    }

    [[nodiscard]]
    std::string_view policy_name(const StartedChild& child) noexcept {
        return ac::protocol::component::to_string(child.termination_policy);
    }

    [[nodiscard]]
    std::wstring policy_description(const StartedChild& child) {
        if (child.termination_policy ==
            ac::protocol::component::TerminationPolicy::force_allowed) {
            return L"force termination allowed";
        }
        return L"graceful shutdown only";
    }

    [[nodiscard]]
    DWORD process_id(const StartedChild& child) noexcept {
        return GetProcessId(child.process);
    }

    [[nodiscard]]
    std::int64_t elapsed_shutdown_ms(
        const std::chrono::steady_clock::time_point shutdown_started
    ) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - shutdown_started
        ).count();
    }

    void log_lingering(
        const std::vector<StartedChild*>& lingering,
        const std::chrono::steady_clock::time_point shutdown_started
    ) {
        for (const auto* child : lingering) {
            auto_core.log_main(
                "Component shutdown deadline expired: name={}, pid={}, "
                "termination_policy={}, elapsed_ms={}",
                child->name,
                process_id(*child),
                policy_name(*child),
                elapsed_shutdown_ms(shutdown_started)
            );
        }
    }

    [[nodiscard]]
    bool all_processes_exited(
        const std::vector<StartedChild*>& lingering
    ) noexcept {
        return std::ranges::all_of(lingering, [](const StartedChild* child) {
            return WaitForSingleObject(child->process, 0) == WAIT_OBJECT_0;
        });
    }

    struct PopupRecoveryState {
        const std::vector<StartedChild*>* lingering {};
        bool all_exited {false};
    };

    HRESULT CALLBACK popup_recovery_callback(
        const HWND window,
        const UINT notification,
        WPARAM,
        LPARAM,
        const LONG_PTR callback_data
    ) {
        if (notification != TDN_TIMER) {
            return S_OK;
        }

        auto& state = *reinterpret_cast<PopupRecoveryState*>(callback_data);
        if (all_processes_exited(*state.lingering)) {
            state.all_exited = true;
            SendMessageW(window, TDM_CLICK_BUTTON, leave_running_button, 0);
        }
        return S_OK;
    }

    [[nodiscard]]
    RecoveryChoice popup_recovery_choice(
        const std::vector<StartedChild*>& lingering
    ) {
        std::wstring message =
            L"Auto Core could not shut down the following components:\n\n";
        for (const auto* child : lingering) {
            message.append(child->name.begin(), child->name.end());
            message += L"        ";
            message += policy_description(*child);
            message += L'\n';
        }
        message +=
            L"\nSome processes may still be completing file, database, or "
            L"network operations.";

        const TASKDIALOG_BUTTON buttons[] {
            {force_terminate_button, L"Force terminate eligible"},
            {leave_running_button, L"Leave running"},
        };
        PopupRecoveryState state {&lingering};
        TASKDIALOGCONFIG config {};
        config.cbSize = sizeof(config);
        config.hInstance = GetModuleHandleW(nullptr);
        config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION |
            TDF_SIZE_TO_CONTENT | TDF_CALLBACK_TIMER;
        config.pszWindowTitle = L"Auto Core Component Shutdown";
        config.pszMainIcon = TD_WARNING_ICON;
        config.pszMainInstruction = L"Some components are still running";
        config.pszContent = message.c_str();
        config.cButtons = static_cast<UINT>(std::size(buttons));
        config.pButtons = buttons;
        config.nDefaultButton = leave_running_button;
        config.pfCallback = popup_recovery_callback;
        config.lpCallbackData = reinterpret_cast<LONG_PTR>(&state);

        int selected = leave_running_button;
        const HRESULT result = TaskDialogIndirect(
            &config,
            &selected,
            nullptr,
            nullptr
        );
        if (FAILED(result)) {
            auto_core.log_main(
                "Component shutdown recovery dialog failed: HRESULT={:#x}",
                static_cast<unsigned long>(result)
            );
            return RecoveryChoice::leave_running;
        }
        if (state.all_exited) {
            return RecoveryChoice::all_exited;
        }
        return selected == force_terminate_button
            ? RecoveryChoice::force_eligible
            : RecoveryChoice::leave_running;
    }

    void log_left_running(
        const StartedChild& child,
        const std::chrono::steady_clock::time_point shutdown_started,
        const std::string_view reason
    ) {
        const DWORD state = WaitForSingleObject(child.process, 0);
        if (state == WAIT_OBJECT_0) {
            auto_core.log_main(
                "Component exited during shutdown recovery: name={}, pid={}, "
                "termination_policy={}, elapsed_ms={}",
                child.name,
                process_id(child),
                policy_name(child),
                elapsed_shutdown_ms(shutdown_started)
            );
            return;
        }
        if (state == WAIT_FAILED) {
            auto_core.log_main(
                "Could not verify component process state: name={}, pid={}, "
                "termination_policy={}, elapsed_ms={}, error={}",
                child.name,
                process_id(child),
                policy_name(child),
                elapsed_shutdown_ms(shutdown_started),
                GetLastError()
            );
        }
        auto_core.log_main(
            "Component left running: name={}, pid={}, termination_policy={}, "
            "elapsed_ms={}, reason={}",
            child.name,
            process_id(child),
            policy_name(child),
            elapsed_shutdown_ms(shutdown_started),
            reason
        );
        auto_core.log_main(
            "ac_{}_pipe may remain occupied until the orphan process exits.",
            child.name
        );
    }

    [[nodiscard]]
    bool force_terminate(
        StartedChild& child,
        const std::chrono::steady_clock::time_point shutdown_started
    ) noexcept {
        const bool eligible = child.termination_policy ==
            ac::protocol::component::TerminationPolicy::force_allowed;
        auto_core.log_main(
            "Force termination eligibility: name={}, pid={}, "
            "termination_policy={}, eligible={}, elapsed_ms={}",
            child.name,
            process_id(child),
            policy_name(child),
            eligible,
            elapsed_shutdown_ms(shutdown_started)
        );
        if (!eligible) {
            log_left_running(child, shutdown_started, "graceful policy");
            return false;
        }

        const DWORD state = WaitForSingleObject(child.process, 0);
        if (state == WAIT_OBJECT_0) {
            auto_core.log_main(
                "Force termination skipped because component exited: name={}, "
                "pid={}, termination_policy={}, elapsed_ms={}",
                child.name,
                process_id(child),
                policy_name(child),
                elapsed_shutdown_ms(shutdown_started)
            );
            return true;
        }
        if (state != WAIT_TIMEOUT) {
            auto_core.log_main(
                "Force termination skipped because process state could not be "
                "verified: name={}, pid={}, termination_policy={}, "
                "elapsed_ms={}, error={}",
                child.name,
                process_id(child),
                policy_name(child),
                elapsed_shutdown_ms(shutdown_started),
                GetLastError()
            );
            return false;
        }

        auto_core.log_main(
            "Force termination attempt: name={}, pid={}, "
            "termination_policy={}, elapsed_ms={}",
            child.name,
            process_id(child),
            policy_name(child),
            elapsed_shutdown_ms(shutdown_started)
        );
        if (TerminateProcess(child.process, 1)) {
            auto_core.log_main(
                "TerminateProcess succeeded: name={}, pid={}, "
                "termination_policy={}, elapsed_ms={}",
                child.name,
                process_id(child),
                policy_name(child),
                elapsed_shutdown_ms(shutdown_started)
            );
            return true;
        }
        auto_core.log_main(
            "TerminateProcess failed: name={}, pid={}, "
            "termination_policy={}, elapsed_ms={}, error={}",
            child.name,
            process_id(child),
            policy_name(child),
            elapsed_shutdown_ms(shutdown_started),
            GetLastError()
        );
        return false;
    }

    [[nodiscard]]
    std::vector<StartedChild*> resolve_force_allowed(
        const std::vector<StartedChild*>& lingering,
        const std::chrono::steady_clock::time_point shutdown_started
    ) {
        std::vector<StartedChild*> unresolved;
        unresolved.reserve(lingering.size());
        for (auto* child : lingering) {
            if (child->termination_policy !=
                ac::protocol::component::TerminationPolicy::force_allowed) {
                unresolved.push_back(child);
                continue;
            }

            if (force_terminate(*child, shutdown_started)) {
                auto_core.log_main(
                    "Component removed from unresolved shutdown set: "
                    "name={}, pid={}, termination_policy={}, elapsed_ms={}",
                    child->name,
                    process_id(*child),
                    policy_name(*child),
                    elapsed_shutdown_ms(shutdown_started)
                );
                continue;
            }

            auto_core.log_main(
                "Component remains unresolved after force termination: "
                "name={}, pid={}, termination_policy={}, elapsed_ms={}",
                child->name,
                process_id(*child),
                policy_name(*child),
                elapsed_shutdown_ms(shutdown_started)
            );
            unresolved.push_back(child);
        }
        return unresolved;
    }

    void apply_recovery_decision(
        const std::vector<StartedChild*>& lingering,
        const bool force_eligible,
        const std::chrono::steady_clock::time_point shutdown_started
    ) {
        for (auto* child : lingering) {
            if (force_eligible) {
                (void)force_terminate(*child, shutdown_started);
            }
            else {
                log_left_running(
                    *child,
                    shutdown_started,
                    "recovery choice"
                );
            }
        }
    }

    [[nodiscard]]
    RecoveryChoice console_recovery_choice(
        const std::vector<StartedChild*>& lingering
    ) {
        const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
        if (input == nullptr || input == INVALID_HANDLE_VALUE) {
            auto_core.log_main(
                "Console shutdown recovery has no valid input handle."
            );
            return RecoveryChoice::continue_waiting;
        }
        (void)FlushConsoleInputBuffer(input);

        std::cout << "\nAuto Core could not shut down:\n\n";
        for (const auto* child : lingering) {
            std::cout
                << child->name
                << "        "
                << (child->termination_policy ==
                        ac::protocol::component::TerminationPolicy::force_allowed
                    ? "force termination allowed"
                    : "graceful shutdown only")
                << '\n';
        }
        std::cout
            << "\nPress Enter to force terminate eligible components.\n"
               "Press any other key to continue waiting.\n"
            << std::flush;

        while (!all_processes_exited(lingering)) {
            std::vector<HANDLE> wait_handles {input};
            wait_handles.reserve(MAXIMUM_WAIT_OBJECTS);
            for (const auto* child : lingering) {
                if (wait_handles.size() == MAXIMUM_WAIT_OBJECTS) {
                    break;
                }
                if (WaitForSingleObject(child->process, 0) == WAIT_TIMEOUT) {
                    wait_handles.push_back(child->process);
                }
            }

            const DWORD wait_result = WaitForMultipleObjects(
                static_cast<DWORD>(wait_handles.size()),
                wait_handles.data(),
                FALSE,
                INFINITE
            );
            if (wait_result == WAIT_FAILED) {
                auto_core.log_main(
                    "Console shutdown recovery wait failed. Error: {}",
                    GetLastError()
                );
                return RecoveryChoice::continue_waiting;
            }
            if (wait_result != WAIT_OBJECT_0) {
                continue;
            }

            INPUT_RECORD record {};
            DWORD read = 0;
            if (!ReadConsoleInputW(input, &record, 1, &read)) {
                auto_core.log_main(
                    "Console shutdown recovery input failed. Error: {}",
                    GetLastError()
                );
                return RecoveryChoice::continue_waiting;
            }
            if (read == 1 && record.EventType == KEY_EVENT &&
                record.Event.KeyEvent.bKeyDown) {
                return record.Event.KeyEvent.wVirtualKeyCode == VK_RETURN
                    ? RecoveryChoice::force_eligible
                    : RecoveryChoice::continue_waiting;
            }
        }
        return RecoveryChoice::all_exited;
    }

    void run_console_recovery(
        std::vector<StartedChild*> lingering,
        const std::chrono::steady_clock::time_point shutdown_started,
        const std::chrono::milliseconds timeout,
        const bool include_logger
    ) {
        while (!lingering.empty()) {
            log_lingering(lingering, shutdown_started);
            const auto choice = console_recovery_choice(lingering);
            if (choice == RecoveryChoice::all_exited) {
                return;
            }
            if (choice == RecoveryChoice::force_eligible) {
                apply_recovery_decision(lingering, true, shutdown_started);
                return;
            }

            std::vector<HANDLE> handles;
            handles.reserve(lingering.size());
            for (const auto* child : lingering) {
                handles.push_back(child->process);
            }
            wait_for_process_exit(
                handles,
                std::chrono::steady_clock::now() + timeout
            );
            lingering = lingering_children(include_logger);
        }
    }

}

ac::main::components::Session::Session(Session&& other) noexcept
    : active_ {other.active_} {
    other.active_ = false;
}

ac::main::components::Session::~Session() {
    if (active_) {
        ac::main::components::shutdown();
    }
}

void ac::main::components::shutdown(const bool allow_recovery_prompt) {
    if (shutdown_complete) {
        return;
    }

    const auto& settings = shutdown_settings();
    const auto shutdown_started = std::chrono::steady_clock::now();
    for (auto child = started_children.rbegin();
         child != started_children.rend();
         ++child) {
        if (*child &&
            !ac::main::components::detail::is_logger((*child)->name)) {
            request_child_shutdown(**child);
        }
    }
    for (auto& child : started_children) {
        if (child &&
            !ac::main::components::detail::is_logger(child->name)) {
            disconnect_child_snapshot(*child);
        }
    }

    std::vector<HANDLE> other_processes;
    other_processes.reserve(started_children.size());
    for (const auto& child : started_children) {
        if (!child || child->process == nullptr ||
            child->process == INVALID_HANDLE_VALUE ||
            ac::main::components::detail::is_logger(child->name)) {
            continue;
        }
        other_processes.push_back(child->process);
    }
    wait_for_process_exit(
        other_processes,
        shutdown_started + settings.timeout
    );
    auto lingering = lingering_children(false);
    if (!lingering.empty()) {
        log_lingering(lingering, shutdown_started);
        lingering = resolve_force_allowed(lingering, shutdown_started);
    }
    if (!lingering.empty()) {
        if (!allow_recovery_prompt) {
            apply_recovery_decision(lingering, false, shutdown_started);
        }
        else if (settings.prompt == DelayedShutdownPrompt::console) {
            run_console_recovery(
                std::move(lingering),
                shutdown_started,
                settings.timeout,
                false
            );
        }
        else {
            const auto choice = popup_recovery_choice(lingering);
            if (choice != RecoveryChoice::all_exited) {
                apply_recovery_decision(
                    lingering,
                    choice == RecoveryChoice::force_eligible,
                    shutdown_started
                );
            }
        }
    }

    StartedChild* logger_child = nullptr;
    for (const auto& child : started_children) {
        if (child && ac::main::components::detail::is_logger(child->name)) {
            logger_child = child.get();
            break;
        }
    }
    if (logger_child != nullptr && logger_child->process != nullptr &&
        logger_child->process != INVALID_HANDLE_VALUE) {
        request_child_shutdown(*logger_child);
        const auto logger_started = std::chrono::steady_clock::now();
        wait_for_process_exit(
            {logger_child->process},
            logger_started +
                ac::main::components::detail::logger_phase_timeout(
                    settings.timeout
                )
        );
        if (WaitForSingleObject(logger_child->process, 0) != WAIT_OBJECT_0) {
            log_lingering({logger_child}, logger_started);
        }
    }

    const bool logger_still_running = logger_child != nullptr &&
        logger_child->process != nullptr &&
        logger_child->process != INVALID_HANDLE_VALUE &&
        WaitForSingleObject(logger_child->process, 0) != WAIT_OBJECT_0;
    if (!logger_still_running &&
        ac::main::components::detail::launch_shutdown_once(
            ac::logging::config::merge_logs_on_shutdown()
        )) {
        const auto executable = ac::paths::bin_directory() /
            ac::protocol::component::executable_name("logger");
        std::error_code exists_error;
        if (!std::filesystem::exists(executable, exists_error)) {
            auto_core.log_print(
                "{} is missing; the shutdown merge was not started.",
                executable.filename()
            );
        }
        else if (!launch_detached_logger_once(executable)) {
            auto_core.log_print(
                "Unable to start {} --once. GetLastError = {}",
                executable.filename(),
                GetLastError()
            );
        }
        else {
            auto_core.log_main(
                "Started logger_ac.exe --once for the shutdown merge."
            );
        }
    }

    for (auto& child : started_children) {
        if (child) {
            close_child_handles(*child);
        }
    }
    started_children.clear();
    shutdown_complete = true;
}

bool ac::main::components::console_shutdown_prompt_enabled() {
    return shutdown_settings().prompt == DelayedShutdownPrompt::console;
}

std::chrono::milliseconds ac::main::components::shutdown_timeout() {
    return shutdown_settings().timeout;
}

void ac::main::components::invoke(
    const std::string_view component_name,
    const std::string_view expression
) {
    auto* child = find_child(component_name);
    if (child == nullptr) {
        auto_core.log_print(
            "{} is unavailable.",
            component_name
        );
        return;
    }

    const std::scoped_lock lock {child->pipe_mutex};
    if (!child->pipe.valid()) {
        auto_core.log_print(
            "{} is unavailable.",
            child->name
        );
        return;
    }

    if (const auto request = ac::pipes::send_pipe_command(
            child->pipe,
            ac::protocol::component::to_wire(
                ac::protocol::component::Request::invoke
            )
        ); !request) {
        auto_core.log_print(
            "{} is unavailable.",
            child->name
        );
        child->pipe.reset();
        return;
    }

    if (const auto payload = ac::pipes::send_string(
            child->pipe, expression
        ); !payload) {
        auto_core.log_print(
            "{} is unavailable.",
            child->name
        );
        child->pipe.reset();
    }
}

void ac::main::components::register_with(
    command_registry::Registry& registry
) {
    std::unordered_set<std::string> preexisting;
    for (const auto& name : registry.registered_names()) {
        preexisting.insert(name);
    }

    for (const auto& child : started_children) {
        if (!child) {
            continue;
        }
        const std::string component = child->name;
        for (const auto& entry : child->catalog) {
            if (registry.contains(entry.name)) {
                if (!preexisting.contains(entry.name)) {
                    auto_core.log_print(
                        "Skipping {} command '{}' because that name is already "
                        "registered.",
                        component,
                        entry.name
                    );
                }
                continue;
            }

            if (entry.accepts_arguments) {
                registry.add_factory(
                    entry.name,
                    [component, command = entry.name](std::string_view arguments) {
                        std::string expression = command;
                        expression += '(';
                        expression += arguments;
                        expression += ')';
                        return [component, expression] {
                            ac::main::components::invoke(component, expression);
                        };
                    },
                    entry.autocomplete
                );
            }
            else {
                registry.add(
                    entry.name,
                    [component, command = entry.name] {
                        ac::main::components::invoke(component, command);
                    }
                );
            }
        }
    }
}

ac::main::components::Session ac::main::components::initialize() {
    Session session;
    started_children.clear();
    shutdown_complete = true;

    const auto& parsed = loaded_catalog();
    if (!parsed.ok) {
        auto_core.log_print("{}", parsed.error);
        return session;
    }

    for (const auto& name : parsed.invalid_names) {
        auto_core.log_print(
            "Invalid component name in components.list [components] is "
            "ignored: {}",
            name
        );
    }
    for (const auto& entry : parsed.malformed_values) {
        auto_core.log_print(
            "Malformed component value in components.list [components] "
            "disables {}: {}",
            entry.name,
            entry.value
        );
    }

    const bool host_logger =
        ac::main::components::detail::host_periodic_logger(
            ac::logging::config::merge_interval_seconds()
        );
    std::vector<std::string> remaining;
    remaining.reserve(parsed.enabled.size());
    bool start_taskbar_first = false;
    for (const auto& name : parsed.enabled) {
        if (name == "logger" && !host_logger) {
            continue;
        }
        if (name == "taskbar") {
            start_taskbar_first = true;
        }
        else {
            remaining.push_back(name);
        }
    }

    if (start_taskbar_first) {
        start_taskbar();
    }
    start_remaining(remaining);

    return session;
}

bool ac::main::components::special_enabled(const std::string_view name) {
    return ac::config::components_list::special_enabled(
        loaded_catalog(),
        name
    );
}
