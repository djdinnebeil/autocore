module;

#include <Windows.h>

module itunes_db_client;

import std;
import auto_core.core.paths;
import auto_core.core.pipes;
import itunes_component;
import itunes_db_protocol;

namespace {

struct Service {
    HANDLE process = nullptr;
    ac::pipes::Pipe pipe;
    bool ready = false;
    std::string startup_error;
    std::mutex mutex;
};

Service& service() {
    static Service instance;
    return instance;
}

std::wstring quote_argument(std::wstring_view value) {
    std::wstring quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back(L'"');
    quoted.append(value);
    quoted.push_back(L'"');
    return quoted;
}

bool process_exited(HANDLE process) {
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

std::expected<std::string, std::string> read_reply(ac::pipes::Pipe& pipe) {
    auto reply = ac::pipes::read_string(pipe);
    if (!reply) {
        if (process_exited(service().process)) {
            return std::unexpected("itunes_db.exe exited during startup.");
        }
        return std::unexpected("iTunes database request failed.");
    }
    return std::move(*reply);
}

} // namespace

namespace itunes::db {

std::expected<void, std::string> start_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (state.ready) {
        return {};
    }
    state.startup_error.clear();
    state.pipe.reset();
    close_process(state);

    const std::filesystem::path executable =
        ac::paths::bin_directory() / "itunes_db.exe";
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
        state.startup_error = "Unable to start itunes_db.exe.";
        itunes_component.log("{}", state.startup_error);
        return std::unexpected(state.startup_error);
    }
    CloseHandle(process.hThread);
    state.process = process.hProcess;

    auto pipe = ac::pipes::connect_to_pipe_server(std::wstring {pipe_name});
    if (!pipe) {
        if (process_exited(state.process)) {
            state.startup_error = "itunes_db.exe exited during startup.";
        }
        else {
            state.startup_error = "itunes_db.exe did not become ready.";
            TerminateProcess(state.process, 1);
        }
        close_process(state);
        itunes_component.log("{}", state.startup_error);
        return std::unexpected(state.startup_error);
    }
    state.pipe = std::move(*pipe);

    if (const auto sent = ac::pipes::send_string(state.pipe, protocol_id); !sent) {
        if (process_exited(state.process)) {
            state.startup_error = "itunes_db.exe exited during startup.";
        }
        else {
            state.startup_error = "itunes_db.exe did not become ready.";
            TerminateProcess(state.process, 1);
        }
        state.pipe.reset();
        close_process(state);
        itunes_component.log("{}", state.startup_error);
        return std::unexpected(state.startup_error);
    }

    auto status = read_reply(state.pipe);
    if (!status || *status != "ok") {
        state.startup_error = status ? "itunes_db.exe did not become ready." : status.error();
        state.pipe.reset();
        if (!process_exited(state.process)) {
            TerminateProcess(state.process, 1);
        }
        close_process(state);
        itunes_component.log("{}", state.startup_error);
        return std::unexpected(state.startup_error);
    }

    state.ready = true;
    return {};
}

std::expected<void, std::string> observe(const Observation& observation) {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (!state.ready) {
        if (!state.startup_error.empty()) {
            return std::unexpected(state.startup_error);
        }
        return std::unexpected("iTunes database service is not running.");
    }

    if (const auto sent = ac::pipes::send_pipe_command(
            state.pipe,
            to_wire(Request::observe)
        );
        !sent) {
        state.ready = false;
        return std::unexpected("iTunes database request failed.");
    }
    const std::string fields[] {
        std::to_string(observation.track_id),
        observation.title,
        observation.artist,
        observation.album,
        std::to_string(observation.duration_seconds),
        std::to_string(observation.credit_seconds),
        observation.observed_at
    };
    for (const std::string& field : fields) {
        if (const auto sent = ac::pipes::send_string(state.pipe, field); !sent) {
            state.ready = false;
            return std::unexpected("iTunes database request failed.");
        }
    }

    auto status = ac::pipes::read_string(state.pipe);
    if (!status) {
        state.ready = false;
        return std::unexpected("iTunes database request failed.");
    }
    if (*status == "error") {
        auto message = ac::pipes::read_string(state.pipe);
        if (!message) {
            state.ready = false;
            return std::unexpected("iTunes database request failed.");
        }
        return std::unexpected(std::move(*message));
    }
    if (*status != "ok") {
        state.ready = false;
        return std::unexpected("iTunes database request failed.");
    }
    return {};
}

void shutdown_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (state.ready) {
        (void)ac::pipes::send_pipe_command(
            state.pipe,
            to_wire(Request::shutdown)
        );
    }
    state.ready = false;
    state.pipe.reset();
    if (state.process != nullptr) {
        WaitForSingleObject(state.process, 5000);
        if (!process_exited(state.process)) {
            TerminateProcess(state.process, 1);
            WaitForSingleObject(state.process, 1000);
        }
        close_process(state);
    }
}

} // namespace itunes::db
