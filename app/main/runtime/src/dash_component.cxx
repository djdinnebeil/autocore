module auto_core.main.components.dash;

import std;
import auto_core.main.application;
import auto_core.core.paths;

import <Windows.h>;

void launch_dash() {
    auto_core.log_main("launch_dash()");

    const auto target = reinterpret_cast<std::uintptr_t>(
        GetForegroundWindow());
    const std::wstring arguments = std::format(
        L"--target {} --parent-pid {}", target, GetCurrentProcessId());
    (void)ac::main::create_process(
        ac::paths::bin_directory() / "dash_ac.exe",
        arguments,
        CREATE_NEW_CONSOLE);
}

void dash::runtime_commands::register_with(
    command_registry::Registry& registry) {
    registry.add("launch_dash", &::launch_dash);
}
