/**
 * \file component_star.ixx
 * \brief Shared menu for `<name>_star.exe`.
 *
 * The menu delegates configuration to `<name>_config.exe` and enablement
 * to `components_editor.exe`. It does not parse `config/<name>.ini` and
 * it does not write `components.list`. A component may pass a baseline
 * probe. When the INI exists and that probe returns true, the menu inserts
 * "Complete initialization" after Configure.
 */
module;

#include "../core/src/components_list_detail.hpp"

export module component_star;

import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.paths;
import components_editor_request;

import <Windows.h>;
import <charconv>;
import <iostream>;

export namespace ac::component_star {

    namespace list = ac::config::components_list;
    namespace request = ac::config::components_request;

    struct ComponentState {
        list::ListedEnablement enablement {list::ListedEnablement::unavailable};
        std::string error;
    };

    /**
     * \brief One component-specific launch inserted after configuration
     *        and before Enable/Disable.
     */
    struct DelegatedAction {
        std::string label;
        std::string executable;
        std::vector<std::string> arguments {};
        /**
         * \brief `CreateProcessW` creation flags. The default keeps the
         * child on the caller's console. Spotify OAuth sets
         * `CREATE_NEW_CONSOLE` so that browser wait stays in its own window.
         */
        DWORD creation_flags {0};
    };

    enum class MenuKind {
        initialize,
        configure,
        complete_initialization,
        delegated,
        toggle,
        exit_star
    };

    struct MenuEntry {
        MenuKind kind {MenuKind::exit_star};
        std::string label;
        std::size_t delegated_index {0};
    };

    struct MenuModel {
        bool configuration_present {false};
        bool enabled {false};
        /**
         * \brief When the INI exists and required baseline files are still
         *        missing. Starter files the user may decline are not this flag.
         */
        bool initialization_incomplete {false};
        std::span<const DelegatedAction> delegated {};
    };

    /**
     * \brief Returns true when required baseline state is still missing.
     *
     * Called only after `config/<name>.ini` is present. A true result inserts
     * "Complete initialization", which launches `<name>_config.exe --init`.
     */
    using BaselineProbe = bool (*)();

    [[nodiscard]]
    inline std::vector<MenuEntry> menu_entries(const MenuModel& model) {
        std::vector<MenuEntry> entries;
        if (model.configuration_present) {
            entries.push_back({MenuKind::configure, "Configure"});
            if (model.initialization_incomplete) {
                entries.push_back({
                    MenuKind::complete_initialization,
                    "Complete initialization"
                });
            }
        }
        else {
            entries.push_back({MenuKind::initialize, "Initialize"});
        }

        for (std::size_t index = 0; index < model.delegated.size(); ++index) {
            entries.push_back({
                MenuKind::delegated,
                model.delegated[index].label,
                index
            });
        }

        entries.push_back({
            MenuKind::toggle,
            model.enabled ? "Disable" : "Enable"
        });
        entries.push_back({MenuKind::exit_star, "Exit"});
        return entries;
    }

    [[nodiscard]]
    inline ComponentState get_state(const std::string_view name) {
        ComponentState state;
        if (!request::is_valid_component_name(name)) {
            state.error = "Invalid component name.";
            return state;
        }

        const auto parsed = list::load(ac::paths::components_list_file());
        state.enablement = list::listed_enablement(parsed, name);
        if (state.enablement == list::ListedEnablement::unavailable) {
            state.error = parsed.error.empty()
                ? "Unable to read components.list. Component state was not determined."
                : parsed.error;
        }
        return state;
    }

    [[nodiscard]]
    inline int set_state(const std::string_view name, const bool enabled) {
        return request::run_component_update(
            name,
            enabled ? request::ListedState::on : request::ListedState::off
        );
    }

    [[nodiscard]]
    inline int toggle_state(const std::string_view name) {
        const auto state = get_state(name);
        if (state.enablement == list::ListedEnablement::unavailable) {
            return 1;
        }
        return set_state(
            name,
            state.enablement != list::ListedEnablement::enabled
        );
    }

    [[nodiscard]]
    int run(
        std::string_view name,
        std::span<const DelegatedAction> delegated = {},
        BaselineProbe baseline_incomplete = nullptr
    );

    [[nodiscard]]
    int run(
        std::string_view name,
        ac::logging::config::LoggingFallback fallback,
        std::span<const DelegatedAction> delegated = {},
        BaselineProbe baseline_incomplete = nullptr
    );

} // namespace ac::component_star

namespace ac::component_star {

    namespace {

        [[nodiscard]]
        std::wstring widen_utf8(const std::string_view text) {
            if (text.empty()) {
                return {};
            }
            const int size = ::MultiByteToWideChar(
                CP_UTF8,
                0,
                text.data(),
                static_cast<int>(text.size()),
                nullptr,
                0
            );
            if (size <= 0) {
                return {};
            }
            std::wstring wide(static_cast<std::size_t>(size), L'\0');
            ::MultiByteToWideChar(
                CP_UTF8,
                0,
                text.data(),
                static_cast<int>(text.size()),
                wide.data(),
                size
            );
            return wide;
        }

        [[nodiscard]]
        std::wstring quote_argument(const std::wstring_view value) {
            std::wstring quoted;
            quoted.reserve(value.size() + 2);
            quoted.push_back(L'"');
            quoted.append(value);
            quoted.push_back(L'"');
            return quoted;
        }

        template <typename Logger>
        [[nodiscard]]
        int launch_executable(
            Logger& logger,
            const std::string_view executable,
            const std::vector<std::string>& arguments,
            const DWORD creation_flags = 0
        ) {
            const auto executable_path =
                ac::paths::bin_directory() / std::string {executable};
            std::error_code error;
            const bool present = std::filesystem::exists(executable_path, error);
            if (error || !present) {
                logger.log_print("Missing {}.", executable_path.string());
                return 1;
            }

            std::wstring command_line = quote_argument(executable_path.wstring());
            for (const auto& argument : arguments) {
                command_line += L' ';
                command_line += quote_argument(widen_utf8(argument));
            }

            STARTUPINFOW startup_info {};
            startup_info.cb = sizeof(startup_info);
            PROCESS_INFORMATION process_info {};
            if (!::CreateProcessW(
                    executable_path.c_str(),
                    command_line.data(),
                    nullptr,
                    nullptr,
                    FALSE,
                    creation_flags,
                    nullptr,
                    executable_path.parent_path().c_str(),
                    &startup_info,
                    &process_info
                )) {
                logger.log_print("Unable to start {}.", executable_path.string());
                return 1;
            }

            ::CloseHandle(process_info.hThread);
            ::WaitForSingleObject(process_info.hProcess, INFINITE);
            DWORD exit_code = 1;
            if (!::GetExitCodeProcess(process_info.hProcess, &exit_code)) {
                ::CloseHandle(process_info.hProcess);
                logger.log_print(
                    "Unable to read the exit code from {}.",
                    executable
                );
                return 1;
            }
            ::CloseHandle(process_info.hProcess);
            const int code = static_cast<int>(exit_code);
            if (code != 0) {
                logger.log_print("{} exited {}.", executable, code);
            }
            return code;
        }

        [[nodiscard]]
        std::optional<bool> configuration_present(
            const std::string_view name,
            std::string& error
        ) {
            const auto path = ac::paths::config_directory() /
                (std::string {name} + ".ini");
            std::error_code code;
            const bool present = std::filesystem::exists(path, code);
            if (code) {
                error = std::format(
                    "Failed to inspect {}: {}",
                    path.string(),
                    code.message()
                );
                return std::nullopt;
            }
            error.clear();
            return present;
        }

        [[nodiscard]]
        bool parse_choice(
            const std::string_view choice,
            const std::size_t count,
            std::size_t& index
        ) {
            if (choice.empty() || count == 0) {
                return false;
            }
            unsigned int value = 0;
            const auto* const end = choice.data() + choice.size();
            const auto result = std::from_chars(choice.data(), end, value);
            if (result.ec != std::errc {} || result.ptr != end || value == 0) {
                return false;
            }
            if (static_cast<std::size_t>(value) > count) {
                return false;
            }
            index = static_cast<std::size_t>(value - 1);
            return true;
        }

    } // namespace

    [[nodiscard]]
    int run(
        const std::string_view name,
        const std::span<const DelegatedAction> delegated,
        const BaselineProbe baseline_incomplete
    ) {
        return run(
            name,
            ac::logging::config::LoggingFallback::global_default,
            delegated,
            baseline_incomplete
        );
    }

    [[nodiscard]]
    int run(
        const std::string_view name,
        const ac::logging::config::LoggingFallback fallback,
        const std::span<const DelegatedAction> delegated,
        const BaselineProbe baseline_incomplete
    ) {
        if (!request::is_valid_component_name(name)) {
            std::cerr << "Invalid component name: " << name << '\n';
            return 1;
        }

        ac::Component star {
            std::string {name} + "_star",
            ac::logging::config::LoggingScope {name, fallback}
        };
        const std::string config_executable = std::string {name} + "_config.exe";

        while (true) {
            const auto state = get_state(name);
            if (state.enablement == list::ListedEnablement::unavailable) {
                star.log_print(std::string_view {state.error});
                return 1;
            }

            std::string inspect_error;
            const auto present = configuration_present(name, inspect_error);
            if (!inspect_error.empty()) {
                star.log_print("{}", inspect_error);
            }

            const bool configuration = present.value_or(true);
            const bool incomplete = configuration &&
                baseline_incomplete != nullptr &&
                baseline_incomplete();
            const auto entries = menu_entries({
                .configuration_present = configuration,
                .enabled = state.enablement == list::ListedEnablement::enabled,
                .initialization_incomplete = incomplete,
                .delegated = delegated
            });

            star.log_print("{}", name);
            for (std::size_t index = 0; index < entries.size(); ++index) {
                star.log_print("{}. {}", index + 1, entries[index].label);
            }
            star.lognl_print("Choice: ");

            std::string line;
            if (!std::getline(std::cin, line)) {
                star.log("Input ended.");
                return 1;
            }
            star.log(std::string_view {line});

            std::size_t selected = 0;
            if (!parse_choice(
                    list::trim_list_line(line),
                    entries.size(),
                    selected
                )) {
                star.log_print(
                    "Enter a number from 1 to {}.",
                    entries.size()
                );
                continue;
            }

            const MenuEntry& entry = entries[selected];
            switch (entry.kind) {
                case MenuKind::initialize:
                case MenuKind::complete_initialization:
                    (void)launch_executable(
                        star,
                        config_executable,
                        {"--init"}
                    );
                    break;
                case MenuKind::configure:
                    (void)launch_executable(star, config_executable, {});
                    break;
                case MenuKind::delegated: {
                    const DelegatedAction& action =
                        delegated[entry.delegated_index];
                    (void)launch_executable(
                        star,
                        action.executable,
                        action.arguments,
                        action.creation_flags
                    );
                    break;
                }
                case MenuKind::toggle: {
                    const int exit_code = set_state(
                        name,
                        state.enablement != list::ListedEnablement::enabled
                    );
                    if (exit_code != 0) {
                        star.log_print(
                            "Failed to update components.list ({}).",
                            exit_code
                        );
                    }
                    break;
                }
                case MenuKind::exit_star:
                    return 0;
            }
        }
    }

} // namespace ac::component_star
