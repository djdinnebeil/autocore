module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

module journal_commands;

import std;
import auto_core.core.paths;
import auto_core.core.process;
import auto_core.core.thread;
import command_registry;
import journal_clock;
import journal_component;
import journal_factories;
import journal_protocol;
import journal_title;

namespace {

std::mutex title_action_mutex;

namespace actions {

void print_extended_timestamp() {
    journal_component().print_and_insert(
        journal_clock::get_extended_timestamp()
    );
}

void launch_journal_config() {
    const auto executable =
        ac::paths::bin_directory() / "journal_config.exe";
    std::wstring command_line = L"\"" + executable.wstring() + L"\"";
    DWORD flags = CREATE_NEW_CONSOLE;
    if (ac::process::in_component_job()) {
        flags |= CREATE_BREAKAWAY_FROM_JOB;
    }

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
        executable.parent_path().c_str(),
        &startup_info,
        &process_info
    )) {
        journal_component().log_print(
            "Unable to start journal_config.exe. Error: {}",
            GetLastError()
        );
        return;
    }

    (void)AllowSetForegroundWindow(process_info.dwProcessId);
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    journal_component().log_main("launch_journal_config()");
}

} // namespace actions

void start_title_action(void (*action)()) {
    std::thread worker([action] {
        const std::scoped_lock lock {title_action_mutex};
        ac::thread::run_with_exception_handling(action, journal_component());
    });
    worker.detach();
}

} // namespace

command_registry::Registry create_journal_command_registry() {
    command_registry::Registry registry;
    registry.add(
        std::string {::print_extended_timestamp.name},
        actions::print_extended_timestamp
    );
    registry.add(std::string {::print_episode_title.name}, [] {
        start_title_action(&journal_title::print_episode_title);
    });
    registry.add(std::string {::save_file_and_create_new_file.name}, [] {
        start_title_action(&journal_title::save_file_and_create_new_file);
    });
    registry.add(
        std::string {::launch_journal_config.name},
        actions::launch_journal_config
    );
    journal::factories::register_factory_commands(registry);
    journal::factories::load_alias_commands(registry);
    return registry;
}
