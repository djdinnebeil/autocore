/**
 * \file component_settings.ixx
 * \brief Shared menu for `<name>_ac.exe --settings`.
 *
 * Configure and Initialize launch `<name>_ac.exe --config`. Enablement goes
 * to `auto_core_components.exe`. It does not parse `config/<name>.ini` and
 * it does not write `components.list`. A component may pass a baseline
 * probe. When the INI exists and that probe returns true, the menu inserts
 * "Complete initialization" after Configure.
 */
module;

#include "../core/component/components_list_detail.hpp"

export module component_settings;

import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.paths;
import components_editor_request;

import <Windows.h>;
import <charconv>;
import <iostream>;

export namespace ac::component_settings {

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

    /**
     * \brief One in-process action inserted after delegated launches and
     *        before Enable/Disable. `invoke` runs on the menu thread.
     */
    struct LocalAction {
        std::string label;
        void (*invoke)() = nullptr;
    };

    enum class MenuKind {
        initialize,
        configure,
        complete_initialization,
        delegated,
        local,
        toggle,
        exit
    };

    struct MenuEntry {
        MenuKind kind {MenuKind::exit};
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
        std::span<const LocalAction> local {};
    };

    /**
     * \brief Returns true when required baseline state is still missing.
     *
     * Called only after `config/<name>.ini` is present. A true result inserts
     * "Complete initialization", which launches `<name>_ac.exe --config --init`.
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

        for (std::size_t index = 0; index < model.local.size(); ++index) {
            entries.push_back({
                MenuKind::local,
                model.local[index].label,
                index
            });
        }

        entries.push_back({
            MenuKind::toggle,
            model.enabled ? "Disable" : "Enable"
        });
        entries.push_back({MenuKind::exit, "Exit"});
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

    /**
     * \brief Executable and arguments for Configure and Initialize.
     *
     * An empty executable means `<name>_ac.exe`. `initialize_arguments`
     * is the full argument list for Initialize and Complete initialization.
     */
    struct ConfigLaunch {
        std::string executable;
        std::vector<std::string> configure_arguments {"--config"};
        std::vector<std::string> initialize_arguments {"--config", "--init"};
    };

    [[nodiscard]]
    int run(
        std::string_view name,
        std::span<const DelegatedAction> delegated = {},
        BaselineProbe baseline_incomplete = nullptr,
        const ConfigLaunch& config_launch = {},
        std::span<const LocalAction> local = {}
    );

    [[nodiscard]]
    int run(
        std::string_view name,
        ac::logging::config::LoggingFallback fallback,
        std::span<const DelegatedAction> delegated = {},
        BaselineProbe baseline_incomplete = nullptr,
        const ConfigLaunch& config_launch = {},
        std::span<const LocalAction> local = {}
    );

} // namespace ac::component_settings

namespace ac::component_settings {

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
            const bool component_executable = [&] {
                constexpr std::string_view suffix = "_ac.exe";
                if (executable.size() < suffix.size()) {
                    return false;
                }
                const auto tail = executable.substr(executable.size() - suffix.size());
                for (std::size_t index = 0; index < suffix.size(); ++index) {
                    char character = tail[index];
                    if (character >= 'A' && character <= 'Z') {
                        character = static_cast<char>(character - 'A' + 'a');
                    }
                    if (character != suffix[index]) {
                        return false;
                    }
                }
                return true;
            }();
            const auto executable_path = (
                component_executable
                    ? ac::paths::component_executable_directory()
                    : ac::paths::app_directory()
            ) / std::string {executable};
            std::error_code error;
            const bool present = std::filesystem::exists(executable_path, error);
            if (error || !present) {
                logger.log_alert("Missing {}.", executable_path.string());
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
                logger.log_alert("Unable to start {}.", executable_path.string());
                return 1;
            }

            ::CloseHandle(process_info.hThread);
            ::WaitForSingleObject(process_info.hProcess, INFINITE);
            DWORD exit_code = 1;
            if (!::GetExitCodeProcess(process_info.hProcess, &exit_code)) {
                ::CloseHandle(process_info.hProcess);
                logger.log_alert(
                    "Unable to read the exit code from {}.",
                    executable
                );
                return 1;
            }
            ::CloseHandle(process_info.hProcess);
            const int code = static_cast<int>(exit_code);
            if (code != 0) {
                logger.log_alert("{} exited {}.", executable, code);
            }
            return code;
        }

        [[nodiscard]]
        std::optional<bool> configuration_present(
            const std::string_view name,
            std::string& error
        ) {
            const auto path = name == "writer"
                ? ac::paths::writer_config_file()
                : ac::paths::component_config_file(name);
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
        const BaselineProbe baseline_incomplete,
        const ConfigLaunch& config_launch,
        const std::span<const LocalAction> local
    ) {
        return run(
            name,
            ac::logging::config::LoggingFallback::global_default,
            delegated,
            baseline_incomplete,
            config_launch,
            local
        );
    }

    [[nodiscard]]
    int run(
        const std::string_view name,
        const ac::logging::config::LoggingFallback fallback,
        const std::span<const DelegatedAction> delegated,
        const BaselineProbe baseline_incomplete,
        const ConfigLaunch& config_launch,
        const std::span<const LocalAction> local
    ) {
        if (!request::is_valid_component_name(name)) {
            std::cerr << "Invalid component name: " << name << '\n';
            return 1;
        }

        ac::Component settings {
            std::string {name} + "_settings",
            ac::logging::config::LoggingScope {name, fallback},
            ac::SharedLog {name}
        };
        ConfigLaunch launch = config_launch;
        if (launch.executable.empty()) {
            launch.executable = std::string {name} + "_ac.exe";
        }
        const std::string& config_executable = launch.executable;

        while (true) {
            const auto state = get_state(name);
            if (state.enablement == list::ListedEnablement::unavailable) {
                settings.log_alert(std::string_view {state.error});
                return 1;
            }

            std::string inspect_error;
            const auto present = configuration_present(name, inspect_error);
            if (!inspect_error.empty()) {
                settings.log_alert("{}", inspect_error);
            }

            const bool configuration = present.value_or(true);
            const bool incomplete = configuration &&
                baseline_incomplete != nullptr &&
                baseline_incomplete();
            const auto entries = menu_entries({
                .configuration_present = configuration,
                .enabled = state.enablement == list::ListedEnablement::enabled,
                .initialization_incomplete = incomplete,
                .delegated = delegated,
                .local = local
            });

            settings.log_alert("{}", name);
            for (std::size_t index = 0; index < entries.size(); ++index) {
                settings.log_alert("{}. {}", index + 1, entries[index].label);
            }
            settings.lognl_alert("Choice: ");

            std::string line;
            if (!std::getline(std::cin, line)) {
                settings.log_debug("Input ended.");
                return 1;
            }
            settings.log_debug(std::string_view {line});

            std::size_t selected = 0;
            if (!parse_choice(
                    list::trim_list_line(line),
                    entries.size(),
                    selected
                )) {
                settings.log_alert(
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
                        settings,
                        config_executable,
                        launch.initialize_arguments
                    );
                    break;
                case MenuKind::configure:
                    (void)launch_executable(
                        settings,
                        config_executable,
                        launch.configure_arguments
                    );
                    break;
                case MenuKind::delegated: {
                    const DelegatedAction& action =
                        delegated[entry.delegated_index];
                    (void)launch_executable(
                        settings,
                        action.executable,
                        action.arguments,
                        action.creation_flags
                    );
                    break;
                }
                case MenuKind::local: {
                    const LocalAction& action = local[entry.delegated_index];
                    if (action.invoke != nullptr) {
                        action.invoke();
                    }
                    break;
                }
                case MenuKind::toggle: {
                    const int exit_code = set_state(
                        name,
                        state.enablement != list::ListedEnablement::enabled
                    );
                    if (exit_code != 0) {
                        settings.log_alert(
                            "Failed to update components.list ({}).",
                            exit_code
                        );
                    }
                    break;
                }
                case MenuKind::exit:
                    return 0;
            }
        }
    }

} // namespace ac::component_settings
