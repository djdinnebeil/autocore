module;

#include <Windows.h>

module journal_db_client;

import std;
import auto_core.core.paths;
import auto_core.core.process;
import journal_component;
import journal_db_protocol;
import journal_db_session;

namespace {

struct Service {
    HANDLE process = nullptr;
    bool owns_process = false;
    bool ready = false;
    std::string startup_error;
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

namespace journal::db {

std::expected<void, std::string> start_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (state.ready) {
        return {};
    }
    state.startup_error.clear();
    state.owns_process = false;
    close_process(state);

    const std::filesystem::path executable =
        ac::paths::bin_directory() / "journal_db.exe";
    std::wstring command =
        quote_argument(executable.wstring()) + L" " +
        quote_argument(L"--serve");

    auto process = ac::process::create_process_with_owner(
        executable,
        command,
        executable.parent_path()
    );
    if (!process) {
        state.startup_error = "Unable to start journal_db.exe.";
        journal_component().log("{}", state.startup_error);
        return std::unexpected(state.startup_error);
    }
    state.process = static_cast<HANDLE>(*process);

    const auto ready = session::probe(state.process);
    if (!ready) {
        state.startup_error = ready.error();
        if (!process_exited(state.process)) {
            TerminateProcess(state.process, 1);
        }
        close_process(state);
        journal_component().log("{}", state.startup_error);
        return std::unexpected(state.startup_error);
    }

    if (process_exited(state.process)) {
        close_process(state);
        state.owns_process = false;
    }
    else {
        state.owns_process = true;
    }
    state.ready = true;
    return {};
}

std::expected<Episode, std::string> allocate_episode(const std::string_view series_key) {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (!state.ready) {
        if (!state.startup_error.empty()) {
            return std::unexpected(state.startup_error);
        }
        return std::unexpected("Journal database service is not running.");
    }
    return session::allocate_episode(series_key);
}

void shutdown_service() {
    auto& state = service();
    const std::lock_guard lock {state.mutex};
    if (state.ready && state.owns_process) {
        (void)session::shutdown();
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
    close_process(state);
}

} // namespace journal::db
