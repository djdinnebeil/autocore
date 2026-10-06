module;

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <shellapi.h>

module auto_core.core.process;

import std;
import auto_core.core.paths;

namespace {

using ac::process::Error;
using ac::process::Handoff;
using ac::process::HandoffResult;
using ac::process::ShellLaunchError;
using ac::process::ShellLaunchFailure;

[[nodiscard]]
std::wstring quote_argument(const std::wstring_view value) {
    if (value.find_first_of(L" \t\"") == std::wstring_view::npos) {
        return std::wstring {value};
    }
    std::wstring quoted;
    quoted.push_back(L'"');
    std::size_t index = 0;
    while (index < value.size()) {
        std::size_t backslashes = 0;
        while (index < value.size() && value[index] == L'\\') {
            ++backslashes;
            ++index;
        }
        if (index == value.size()) {
            quoted.append(backslashes * 2, L'\\');
            break;
        }
        if (value[index] == L'"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(L'"');
        }
        else {
            quoted.append(backslashes, L'\\');
            quoted.push_back(value[index]);
        }
        ++index;
    }
    quoted.push_back(L'"');
    return quoted;
}

void close_handle(HANDLE handle) noexcept {
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
    }
}

[[nodiscard]]
bool process_is_alive(const HANDLE process) noexcept {
    return process != nullptr &&
        process != INVALID_HANDLE_VALUE &&
        WaitForSingleObject(process, 0) == WAIT_TIMEOUT;
}

[[nodiscard]]
bool duplicate_job_into(
    const HANDLE job,
    const HANDLE process
) noexcept {
    if (!process_is_alive(process)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }
    BOOL member = FALSE;
    if (IsProcessInJob(process, job, &member) == FALSE) {
        return false;
    }
    if (member == FALSE) {
        SetLastError(ERROR_NOT_FOUND);
        return false;
    }
    HANDLE remote = nullptr;
    if (DuplicateHandle(
            GetCurrentProcess(),
            job,
            process,
            &remote,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
        ) == FALSE || remote == nullptr) {
        return false;
    }
    member = FALSE;
    if (IsProcessInJob(process, job, &member) == FALSE || member == FALSE) {
        return false;
    }
    return true;
}

[[nodiscard]]
HandoffResult handoff_to_verified_member(const HANDLE job) noexcept {
    constexpr std::size_t initial_slots = 8;
    std::vector<std::byte> buffer(
        sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) +
        sizeof(ULONG_PTR) * initial_slots
    );
    auto* list = reinterpret_cast<JOBOBJECT_BASIC_PROCESS_ID_LIST*>(
        buffer.data()
    );
    while (QueryInformationJobObject(
               job,
               JobObjectBasicProcessIdList,
               list,
               static_cast<DWORD>(buffer.size()),
               nullptr
           ) == FALSE) {
        if (GetLastError() != ERROR_MORE_DATA) {
            return HandoffResult {
                Handoff::failed,
                GetLastError(),
                nullptr
            };
        }
        const auto count = static_cast<std::size_t>(list->NumberOfAssignedProcesses);
        buffer.assign(
            sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + sizeof(ULONG_PTR) * count,
            std::byte {}
        );
        list = reinterpret_cast<JOBOBJECT_BASIC_PROCESS_ID_LIST*>(buffer.data());
    }

    HandoffResult failure {Handoff::failed, ERROR_NOT_FOUND, nullptr};
    for (DWORD index = 0; index < list->NumberOfProcessIdsInList; ++index) {
        const auto process_id = static_cast<DWORD>(list->ProcessIdList[index]);
        const HANDLE process = OpenProcess(
            PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION |
                PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
            FALSE,
            process_id
        );
        if (process == nullptr) {
            failure.system_error = GetLastError();
            continue;
        }
        if (duplicate_job_into(job, process)) {
            return HandoffResult {Handoff::transferred, 0, process};
        }
        failure.system_error = GetLastError();
        CloseHandle(process);
    }
    return failure;
}

} // namespace

namespace ac::process {

std::wstring component_job_name(const unsigned long process_id) {
    return L"Local\\AutoCoreComponentJob_" + std::to_wstring(process_id);
}

bool in_component_job() noexcept {
    const HANDLE job = OpenJobObjectW(
        JOB_OBJECT_QUERY,
        FALSE,
        component_job_name(GetCurrentProcessId()).c_str()
    );
    if (job == nullptr) {
        return false;
    }
    BOOL member = FALSE;
    const BOOL queried = IsProcessInJob(GetCurrentProcess(), job, &member);
    CloseHandle(job);
    return queried != FALSE && member != FALSE;
}

std::expected<void*, Error> create_component_job(
    const unsigned long process_id
) {
    const HANDLE job = CreateJobObjectW(
        nullptr,
        component_job_name(process_id).c_str()
    );
    if (job == nullptr) {
        return std::unexpected(Error {GetLastError()});
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits {};
    limits.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_BREAKAWAY_OK;
    if (SetInformationJobObject(
            job,
            JobObjectExtendedLimitInformation,
            &limits,
            sizeof(limits)
        ) == FALSE) {
        const Error error {GetLastError()};
        CloseHandle(job);
        return std::unexpected(error);
    }
    return job;
}

std::expected<std::size_t, Error> job_active_count(void* const job) noexcept {
    if (job == nullptr) {
        return std::unexpected(Error {ERROR_INVALID_HANDLE});
    }
    JOBOBJECT_BASIC_PROCESS_ID_LIST list {};
    if (QueryInformationJobObject(
            job,
            JobObjectBasicProcessIdList,
            &list,
            sizeof(list),
            nullptr
        ) != FALSE) {
        return static_cast<std::size_t>(list.NumberOfAssignedProcesses);
    }
    if (GetLastError() == ERROR_MORE_DATA) {
        return static_cast<std::size_t>(list.NumberOfAssignedProcesses);
    }
    return std::unexpected(Error {GetLastError()});
}

bool terminate_job(void* const job, const unsigned int exit_code) noexcept {
    return job != nullptr &&
        TerminateJobObject(job, static_cast<UINT>(exit_code)) != FALSE;
}

bool try_clear_kill_on_job_close(void* const job) noexcept {
    if (job == nullptr) {
        return false;
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits {};
    if (QueryInformationJobObject(
            job,
            JobObjectExtendedLimitInformation,
            &limits,
            sizeof(limits),
            nullptr
        ) == FALSE) {
        return false;
    }
    limits.BasicLimitInformation.LimitFlags &=
        ~static_cast<DWORD>(JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE);
    return SetInformationJobObject(
        job,
        JobObjectExtendedLimitInformation,
        &limits,
        sizeof(limits)
    ) != FALSE;
}

std::expected<void, Error> resume_in_job(
    void* const job,
    void* const process,
    void*& thread
) noexcept {
    const auto fail = [&](const unsigned long error) {
        if (process != nullptr) {
            (void)TerminateProcess(process, 1);
        }
        close_handle(static_cast<HANDLE>(thread));
        thread = nullptr;
        return std::unexpected(Error {error});
    };

    if (job == nullptr || process == nullptr || thread == nullptr) {
        return fail(ERROR_INVALID_HANDLE);
    }
    if (AssignProcessToJobObject(job, process) == FALSE) {
        return fail(GetLastError());
    }
    if (ResumeThread(static_cast<HANDLE>(thread)) == static_cast<DWORD>(-1)) {
        return fail(GetLastError());
    }
    close_handle(static_cast<HANDLE>(thread));
    thread = nullptr;
    return {};
}

std::expected<void*, Error> create_process_with_owner(
    const std::filesystem::path& executable,
    std::wstring command_line,
    const std::filesystem::path& working_directory
) {
    HANDLE owner = nullptr;
    if (DuplicateHandle(
            GetCurrentProcess(),
            GetCurrentProcess(),
            GetCurrentProcess(),
            &owner,
            SYNCHRONIZE,
            TRUE,
            0
        ) == FALSE || owner == nullptr) {
        return std::unexpected(Error {GetLastError()});
    }

    command_line += L" --owner-handle ";
    command_line += std::to_wstring(reinterpret_cast<std::uintptr_t>(owner));

    SIZE_T attribute_size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attribute_size);
    auto* attributes = static_cast<PPROC_THREAD_ATTRIBUTE_LIST>(
        HeapAlloc(GetProcessHeap(), 0, attribute_size)
    );
    if (attributes == nullptr ||
        InitializeProcThreadAttributeList(attributes, 1, 0, &attribute_size) == FALSE) {
        const Error error {GetLastError()};
        if (attributes != nullptr) {
            HeapFree(GetProcessHeap(), 0, attributes);
        }
        CloseHandle(owner);
        return std::unexpected(error);
    }

    const HANDLE inherited[] {owner};
    const BOOL updated = UpdateProcThreadAttribute(
        attributes,
        0,
        PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        const_cast<HANDLE*>(inherited),
        sizeof(inherited),
        nullptr,
        nullptr
    );
    if (updated == FALSE) {
        const Error error {GetLastError()};
        DeleteProcThreadAttributeList(attributes);
        HeapFree(GetProcessHeap(), 0, attributes);
        CloseHandle(owner);
        return std::unexpected(error);
    }

    STARTUPINFOEXW startup {};
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = attributes;
    PROCESS_INFORMATION process {};
    const BOOL created = CreateProcessW(
        executable.c_str(),
        command_line.data(),
        nullptr,
        nullptr,
        TRUE,
        EXTENDED_STARTUPINFO_PRESENT,
        nullptr,
        working_directory.c_str(),
        &startup.StartupInfo,
        &process
    );
    const auto create_error = GetLastError();
    DeleteProcThreadAttributeList(attributes);
    HeapFree(GetProcessHeap(), 0, attributes);
    CloseHandle(owner);
    if (created == FALSE) {
        return std::unexpected(Error {create_error});
    }
    CloseHandle(process.hThread);
    return process.hProcess;
}

std::expected<void*, Error> parse_owner_handle(const std::string_view text) {
    if (text.empty()) {
        return std::unexpected(Error {ERROR_INVALID_HANDLE});
    }
    std::uint64_t value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc {} || parsed.ptr != text.data() + text.size() ||
        value == 0 || value > (std::numeric_limits<std::uintptr_t>::max)()) {
        return std::unexpected(Error {ERROR_INVALID_HANDLE});
    }
    auto* const handle = reinterpret_cast<HANDLE>(
        static_cast<std::uintptr_t>(value)
    );
    if (WaitForSingleObject(handle, 0) == WAIT_FAILED) {
        return std::unexpected(Error {GetLastError()});
    }
    return handle;
}

struct OwnerWatch::State {
    HANDLE owner {nullptr};
    HANDLE serve_thread {nullptr};
    HANDLE stop_event {nullptr};
    std::thread thread;
    std::function<void()> on_owner_exit;

    ~State() {
        if (stop_event != nullptr) {
            SetEvent(stop_event);
        }
        if (thread.joinable()) {
            thread.join();
        }
        close_handle(stop_event);
        close_handle(serve_thread);
        close_handle(owner);
    }
};

OwnerWatch::OwnerWatch() noexcept = default;

OwnerWatch::OwnerWatch(void* const owner, std::function<void()> on_owner_exit) {
    if (owner == nullptr) {
        return;
    }
    auto* state = new State;
    state->on_owner_exit = std::move(on_owner_exit);
    if (DuplicateHandle(
            GetCurrentProcess(),
            owner,
            GetCurrentProcess(),
            &state->owner,
            SYNCHRONIZE,
            FALSE,
            0
        ) == FALSE) {
        delete state;
        return;
    }
    if (DuplicateHandle(
            GetCurrentProcess(),
            GetCurrentThread(),
            GetCurrentProcess(),
            &state->serve_thread,
            THREAD_TERMINATE,
            FALSE,
            0
        ) == FALSE) {
        delete state;
        return;
    }
    state->stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (state->stop_event == nullptr) {
        delete state;
        return;
    }
    state->thread = std::thread([state] {
        const HANDLE waits[] {state->owner, state->stop_event};
        const DWORD result = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
        if (result != WAIT_OBJECT_0) {
            return;
        }
        try {
            if (state->on_owner_exit) {
                state->on_owner_exit();
            }
        }
        catch (...) {
        }
        while (WaitForSingleObject(state->stop_event, 0) == WAIT_TIMEOUT) {
            if (CancelSynchronousIo(state->serve_thread) == FALSE &&
                GetLastError() != ERROR_NOT_FOUND) {
                // The serve thread may not have entered its blocking call yet.
            }
            if (WaitForSingleObject(state->stop_event, 20) == WAIT_OBJECT_0) {
                break;
            }
        }
    });
    state_ = state;
}

OwnerWatch::~OwnerWatch() {
    delete state_;
}

OwnerWatch::OwnerWatch(OwnerWatch&& other) noexcept
    : state_ {other.state_} {
    other.state_ = nullptr;
}

OwnerWatch& OwnerWatch::operator=(OwnerWatch&& other) noexcept {
    if (this != &other) {
        delete state_;
        state_ = other.state_;
        other.state_ = nullptr;
    }
    return *this;
}

bool OwnerWatch::active() const noexcept {
    return state_ != nullptr;
}

std::expected<void, ShellLaunchError> shell_launch_outside_job(
    const std::wstring_view verb,
    const std::wstring_view file,
    const std::wstring_view parameters,
    const std::wstring_view directory
) {
    const auto launcher = ac::paths::bin_directory() / L"auto_core_shell_launcher.exe";
    std::error_code exists_error;
    if (!std::filesystem::exists(launcher, exists_error) || exists_error) {
        return std::unexpected(ShellLaunchError {
            ShellLaunchFailure::launcher_missing,
            ERROR_FILE_NOT_FOUND
        });
    }

    std::wstring command = quote_argument(launcher.wstring());
    if (!verb.empty()) {
        command += L" --verb ";
        command += quote_argument(verb);
    }
    command += L" --file ";
    command += quote_argument(file);
    if (!parameters.empty()) {
        command += L" --parameters ";
        command += quote_argument(parameters);
    }
    if (!directory.empty()) {
        command += L" --directory ";
        command += quote_argument(directory);
    }

    BOOL in_any_job = FALSE;
    (void)IsProcessInJob(GetCurrentProcess(), nullptr, &in_any_job);
    const bool component_job = in_component_job();
    DWORD flags = 0;
    if (in_any_job != FALSE) {
        flags |= CREATE_BREAKAWAY_FROM_JOB;
    }

    const auto create = [&](const DWORD creation_flags)
        -> std::expected<void, ShellLaunchError> {
        STARTUPINFOW startup {};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process {};
        if (CreateProcessW(
                launcher.c_str(),
                command.data(),
                nullptr,
                nullptr,
                FALSE,
                creation_flags,
                nullptr,
                launcher.parent_path().c_str(),
                &startup,
                &process
            ) == FALSE) {
            return std::unexpected(ShellLaunchError {
                ShellLaunchFailure::create_failed,
                GetLastError()
            });
        }
        CloseHandle(process.hThread);
        const DWORD wait = WaitForSingleObject(process.hProcess, INFINITE);
        DWORD exit_code = 1;
        (void)GetExitCodeProcess(process.hProcess, &exit_code);
        CloseHandle(process.hProcess);
        if (wait != WAIT_OBJECT_0 || exit_code != 0) {
            return std::unexpected(ShellLaunchError {
                ShellLaunchFailure::launcher_failed,
                exit_code
            });
        }
        return std::expected<void, ShellLaunchError> {};
    };

    auto created = create(flags);
    if (created) {
        return {};
    }
    if ((flags & CREATE_BREAKAWAY_FROM_JOB) != 0 && component_job) {
        return std::unexpected(ShellLaunchError {
            ShellLaunchFailure::breakaway_denied,
            created.error().system_error
        });
    }
    if ((flags & CREATE_BREAKAWAY_FROM_JOB) != 0 && !component_job) {
        const std::wstring verb_text {verb};
        const std::wstring file_text {file};
        const std::wstring parameter_text {parameters};
        const std::wstring directory_text {directory};
        SHELLEXECUTEINFOW execution {};
        execution.cbSize = sizeof(execution);
        execution.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
        execution.lpVerb = verb_text.empty() ? nullptr : verb_text.c_str();
        execution.lpFile = file_text.c_str();
        execution.lpParameters =
            parameter_text.empty() ? nullptr : parameter_text.c_str();
        execution.lpDirectory =
            directory_text.empty() ? nullptr : directory_text.c_str();
        execution.nShow = SW_SHOWNORMAL;
        if (ShellExecuteExW(&execution) == FALSE ||
            reinterpret_cast<std::intptr_t>(execution.hInstApp) <= 32) {
            return std::unexpected(ShellLaunchError {
                ShellLaunchFailure::launcher_failed,
                GetLastError()
            });
        }
        if (execution.hProcess != nullptr) {
            (void)WaitForInputIdle(execution.hProcess, 1000);
            CloseHandle(execution.hProcess);
        }
        return {};
    }
    return std::unexpected(created.error());
}

HandoffResult handoff_job(void* const job, void* const root_process) noexcept {
    if (job == nullptr) {
        return HandoffResult {Handoff::failed, ERROR_INVALID_HANDLE, nullptr};
    }
    const auto active = job_active_count(job);
    if (!active) {
        return HandoffResult {Handoff::failed, active.error().system_error, nullptr};
    }
    if (*active == 0) {
        return HandoffResult {Handoff::already_empty, 0, nullptr};
    }
    if (process_is_alive(static_cast<HANDLE>(root_process))) {
        if (duplicate_job_into(job, static_cast<HANDLE>(root_process))) {
            return HandoffResult {Handoff::transferred, 0, nullptr};
        }
        return HandoffResult {Handoff::failed, GetLastError(), nullptr};
    }
    return handoff_to_verified_member(static_cast<HANDLE>(job));
}

} // namespace ac::process
