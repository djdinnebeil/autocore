/**
 * \file components_editor_request.ixx
 * \brief Launch `components_editor.exe` from a child helper.
 *        `--component <name>` registers a name. `--on` or `--off` with
 *        that flag sets an explicit `components.list` state.
 */
export module components_editor_request;

import std;
import auto_core.core.paths;

import <Windows.h>;
import <iostream>;

export namespace ac::config::components_request {

    inline constexpr std::string_view executable_name = "components_editor.exe";
    inline constexpr std::string_view flag = "--component";
    inline constexpr std::string_view on_flag = "--on";
    inline constexpr std::string_view off_flag = "--off";
    inline constexpr std::string_view initialize_flag = "--initialize";
    inline constexpr std::string_view init_flag = "--init";
    inline constexpr std::string_view seed_flag = "--seed";

    enum class ListedState {
        unchanged,
        on,
        off
    };

    /**
     * \brief Flags for a `_config.exe` launch.
     *
     * `--init` is interactive initialization. `--initialize` is a
     * compatibility alias of `--init`. `--seed` writes default
     * configuration only when the owned file is missing. Order does
     * not matter. `--init --seed` is the combined mode.
     */
    struct ConfigLaunch {
        bool init {false};
        bool seed {false};
    };

    template <typename Logger>
    inline void log_config_request(
        Logger& logger,
        const ConfigLaunch& launch
    ) {
        if (launch.init && launch.seed) {
            logger.log_print("Initialization requested with seed mode");
            return;
        }
        if (launch.seed) {
            logger.log_print("Seed requested");
            return;
        }
        if (launch.init) {
            logger.log_print("Initialization requested");
        }
    }

    template <typename Logger>
    inline void log_seed_skipped(
        Logger& logger,
        const std::string_view file
    ) {
        logger.log_print("Seed skipped; {} already exists", file);
    }

    template <typename Logger>
    inline void log_initialization_skipped(
        Logger& logger,
        const std::string_view file
    ) {
        logger.log_print("Initialization skipped; {} already exists", file);
    }

    template <typename Logger>
    inline void log_writing_defaults(Logger& logger) {
        logger.log_print("Writing default configuration");
    }

    template <typename Logger>
    inline void log_configuration_initialized(Logger& logger) {
        logger.log_print("Configuration initialized successfully");
    }

    template <typename Logger>
    inline void log_configuration_missing(Logger& logger) {
        logger.log_print("Configuration file missing; starting initialization");
    }

    [[nodiscard]]
    inline std::optional<ConfigLaunch> parse_config_launch(
        const int argc,
        char* argv[]
    ) {
        ConfigLaunch launch;
        for (int i = 1; i < argc; ++i) {
            const std::string_view argument {argv[i]};
            if (argument == init_flag || argument == initialize_flag) {
                launch.init = true;
                continue;
            }
            if (argument == seed_flag) {
                launch.seed = true;
                continue;
            }
            std::cerr << "Unknown argument: " << argument << '\n';
            return std::nullopt;
        }
        return launch;
    }

    [[nodiscard]]
    inline std::optional<ConfigLaunch> parse_config_launch(
        const int argc,
        wchar_t* argv[]
    ) {
        ConfigLaunch launch;
        for (int i = 1; i < argc; ++i) {
            const std::wstring_view argument {argv[i]};
            if (argument == L"--init" || argument == L"--initialize") {
                launch.init = true;
                continue;
            }
            if (argument == L"--seed") {
                launch.seed = true;
                continue;
            }
            std::wcerr << L"Unknown argument: " << argument << L'\n';
            return std::nullopt;
        }
        return launch;
    }

    /**
     * \brief True when `--init` or its `--initialize` alias is present.
     *
     * This is not a seed check. Seeding is `ConfigLaunch::seed`.
     */
    [[nodiscard]]
    inline bool is_initialize_run(const int argc, char* argv[]) noexcept {
        for (int i = 1; i < argc; ++i) {
            const std::string_view argument {argv[i]};
            if (argument == init_flag || argument == initialize_flag) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    inline bool is_initialize_run(const int argc, wchar_t* argv[]) noexcept {
        for (int i = 1; i < argc; ++i) {
            const std::wstring_view argument {argv[i]};
            if (argument == L"--init" || argument == L"--initialize") {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    inline bool is_valid_component_name(const std::string_view name) noexcept {
        if (name.empty() || name.front() < 'a' || name.front() > 'z') {
            return false;
        }
        for (const char character : name) {
            const bool letter = character >= 'a' && character <= 'z';
            const bool digit = character >= '0' && character <= '9';
            if (!letter && !digit && character != '_') {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]]
    inline int run_component_update(
        const std::string_view name,
        const ListedState state = ListedState::unchanged
    ) {
        if (!is_valid_component_name(name)) {
            std::cerr
                << "Invalid component name for components.list: "
                << name
                << '\n';
            return 1;
        }

        const auto executable_path =
            ac::paths::bin_directory() / executable_name;
        STARTUPINFOW startup_info {};
        startup_info.cb = sizeof(startup_info);
        PROCESS_INFORMATION process_info {};

        std::wstring command_line = L"\"" + executable_path.wstring() + L"\" ";
        command_line.append(flag.begin(), flag.end());
        command_line += L' ';
        command_line.append(name.begin(), name.end());
        if (state == ListedState::on) {
            command_line += L' ';
            command_line.append(on_flag.begin(), on_flag.end());
        }
        else if (state == ListedState::off) {
            command_line += L' ';
            command_line.append(off_flag.begin(), off_flag.end());
        }

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
            std::cerr << "Unable to start " << executable_path.string() << '\n';
            return 1;
        }

        CloseHandle(process_info.hThread);
        WaitForSingleObject(process_info.hProcess, INFINITE);
        DWORD exit_code = 1;
        if (!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
            CloseHandle(process_info.hProcess);
            return 1;
        }
        CloseHandle(process_info.hProcess);
        return static_cast<int>(exit_code);
    }

} // namespace ac::config::components_request
