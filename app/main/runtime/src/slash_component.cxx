module auto_core.main.components.slash;

import std;
import auto_core.main.application;
import auto_core.core.encoding;
import auto_core.core.paths;
import slash_protocol;

import <Windows.h>;

namespace {

void call_slash_exe(
    const std::filesystem::path& executable_path,
    const std::string_view command_name
) {
    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info {};
    std::wstring command_line =
        L"\"" + executable_path.wstring() + L"\" " +
        ac::encoding::to_utf16(command_name);

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
        const DWORD error = GetLastError();

        auto_core.print(
            "Unable to start '{}'. Error: {}",
            executable_path,
            error
        );

        return;
    }

    const DWORD wait_result = WaitForSingleObject(
        process_info.hProcess,
        INFINITE
    );

    if (wait_result == WAIT_FAILED) {
        const DWORD error = GetLastError();

        auto_core.print(
            "Unable to wait for '{}'. Error: {}",
            executable_path,
            error
        );
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
}

} // namespace

std::function<void()> slash_component_command(
    const ac::protocol::slash::CommandName command
) {
    return [name = std::string {command.name}] {
        auto_core.log_main(
            "Starting slash_ac.exe command: {}",
            name
        );

        call_slash_exe(
            ac::paths::bin_directory() / "slash_ac.exe",
            name
        );
    };
}

void slash_component::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    std::unordered_set<std::string> names;
    std::ifstream input(
        ac::paths::keymap_components_directory() /
        ac::protocol::slash::manifest_filename
    );
    const bool manifest_available = input.is_open();
    std::string name;

    while (std::getline(input, name)) {
        if (!name.empty() && name.back() == '\r') {
            name.pop_back();
        }
        if (name.starts_with("\xEF\xBB\xBF")) {
            name.erase(0, 3);
        }
        if (!name.empty()) {
            names.emplace(std::move(name));
        }
    }

    if (!manifest_available) {
        for (const auto command : slash::commands::all) {
            names.emplace(command.name);
        }
    }

    for (const std::string& value : names) {
        registry.add(
            value,
            slash_component_command({value})
        );
    }
}
