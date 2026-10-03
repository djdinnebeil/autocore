module;

#include <Windows.h>

module journal_cloud_client;

import std;
import auto_core.core.paths;
import auto_core.core.pipes;
import journal_cloud_protocol;
import journal_component;

namespace {

struct Service {
    HANDLE process = nullptr;
    ac::pipes::Pipe pipe;
    bool owns_process = false;
    bool ready = false;
    std::mutex mutex;
};

Service& service() {
    static Service instance;
    return instance;
}

std::wstring quote_argument(const std::wstring_view value) {
    std::wstring quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back(L'"');
    quoted.append(value);
    quoted.push_back(L'"');
    return quoted;
}

bool process_exited(const HANDLE process) {
    if (process == nullptr) {
        return true;
    }
    DWORD exit_code = STILL_ACTIVE;
    if (!GetExitCodeProcess(process, &exit_code)) {
        return true;
    }
    return exit_code != STILL_ACTIVE;
}

void close_process(Service& state) {
    if (state.process != nullptr) {
        CloseHandle(state.process);
        state.process = nullptr;
    }
}

} // namespace

namespace journal::cloud {

std::expected<void, std::string> start_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (state.ready) {
        return {};
    }
    state.pipe.reset();
    state.owns_process = false;
    close_process(state);

    const std::filesystem::path executable =
        ac::paths::bin_directory() / "journal_cloud.exe";
    std::wstring command =
        quote_argument(executable.wstring()) + L" " +
        quote_argument(L"--serve");

    STARTUPINFOW startup {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process {};
    if (!CreateProcessW(
            executable.c_str(),
            command.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            executable.parent_path().c_str(),
            &startup,
            &process
        )) {
        const std::string message = "Unable to start journal_cloud.exe.";
        journal_component().log("{}", message);
        return std::unexpected(message);
    }
    CloseHandle(process.hThread);
    state.process = process.hProcess;

    auto pipe = ac::pipes::connect_to_pipe_server(std::wstring {pipe_name});
    if (!pipe) {
        const std::string message = process_exited(state.process)
            ? "journal_cloud.exe exited during startup."
            : "journal_cloud.exe did not become ready.";
        if (!process_exited(state.process)) {
            TerminateProcess(state.process, 1);
        }
        close_process(state);
        journal_component().log("{}", message);
        return std::unexpected(message);
    }
    state.pipe = std::move(*pipe);
    if (const auto sent = ac::pipes::send_string(state.pipe, protocol_id); !sent) {
        const std::string message = "journal_cloud.exe did not become ready.";
        if (!process_exited(state.process)) {
            TerminateProcess(state.process, 1);
        }
        state.pipe.reset();
        close_process(state);
        journal_component().log("{}", message);
        return std::unexpected(message);
    }
    auto status = ac::pipes::read_string(state.pipe);
    if (!status || *status != "ok") {
        if (status && *status == "protocol") {
            (void)ac::pipes::read_string(state.pipe);
        }
        const std::string message = "Journal cloud protocol mismatch.";
        state.pipe.reset();
        if (!process_exited(state.process)) {
            TerminateProcess(state.process, 1);
        }
        close_process(state);
        journal_component().log("{}", message);
        return std::unexpected(message);
    }
    if (process_exited(state.process)) {
        state.pipe.reset();
        close_process(state);
        state.owns_process = false;
        state.ready = false;
        const std::string message = "journal_cloud.exe exited during startup.";
        journal_component().log("{}", message);
        return std::unexpected(message);
    }
    state.owns_process = true;
    state.ready = true;
    return {};
}

bool service_running() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    return state.ready;
}

std::expected<void, std::string> push(const std::string_view name, const int count) {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (!state.ready) {
        return {};
    }
    if (const auto sent = ac::pipes::send_pipe_command(state.pipe, to_wire(Request::push));
        !sent) {
        state.ready = false;
        return std::unexpected("Journal cloud request failed.");
    }
    if (const auto sent = ac::pipes::send_string(state.pipe, name); !sent) {
        state.ready = false;
        return std::unexpected("Journal cloud request failed.");
    }
    if (const auto sent = ac::pipes::send_string(state.pipe, std::to_string(count));
        !sent) {
        state.ready = false;
        return std::unexpected("Journal cloud request failed.");
    }
    auto status = ac::pipes::read_string(state.pipe);
    if (!status) {
        state.ready = false;
        return std::unexpected("Journal cloud request failed.");
    }
    if (*status == "error") {
        auto message = ac::pipes::read_string(state.pipe);
        if (!message) {
            state.ready = false;
            return std::unexpected("Journal cloud request failed.");
        }
        return std::unexpected(std::move(*message));
    }
    if (*status != "ok") {
        state.ready = false;
        return std::unexpected("Journal cloud request failed.");
    }
    return {};
}

void shutdown_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (state.ready && state.owns_process) {
        (void)ac::pipes::send_pipe_command(state.pipe, to_wire(Request::shutdown));
        if (state.process != nullptr) {
            WaitForSingleObject(state.process, 5000);
            if (!process_exited(state.process)) {
                TerminateProcess(state.process, 1);
                WaitForSingleObject(state.process, 1000);
            }
        }
    }
    state.ready = false;
    state.owns_process = false;
    state.pipe.reset();
    close_process(state);
}

} // namespace journal::cloud
