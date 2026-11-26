/**
 * \file main.cxx
 * \brief First-run orchestration for Auto Core. Does not write configuration files.
 *
 * A missing logging_policy.ini is seeded enabled before any normal logging
 * call. An existing policy is resolved and left unchanged. The Auto Core
 * menu then selects Main configuration. Configure asks whether to keep
 * logging only when this run created the policy. `auto_core.ini` is created
 * during initialization and retained as the sentinel only if the required
 * initialization sequence succeeds. When this run creates
 * `config/auto_core.ini`, a failed or cancelled required step removes that
 * file before exit. Interactive runs then ask whether to seed, configure,
 * or skip component initialization. `auto_core_components.exe --edit --seed`
 * runs after that stage.
 */
import std;
import auto_core.core.component;
import auto_core.core.encoding;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core_initialization;
import components_editor_request;
import logging_bootstrap;

import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace boot = ac::main::logging_bootstrap;
namespace sequence = ac::main::init;

namespace {

ac::Component auto_core_init {"auto_core_init", "auto_core"};

[[nodiscard]]
int run_owner(
    const std::string_view executable_name,
    const std::wstring_view arguments
) {
    std::string command {executable_name};
    if (!arguments.empty()) {
        command += ' ';
        command += ac::encoding::to_utf8(arguments);
    }

    const auto path = ac::paths::bin_directory() / executable_name;
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error || !present) {
        auto_core_init.print("{} was not found.", executable_name);
        return 1;
    }

    auto_core_init.log_debug("Launching {}", command);
    const int code = cfg::run_config_exe(
        executable_name,
        std::wstring {arguments}
    );
    if (code != 0) {
        auto_core_init.print(
            "{} exited with code {}.",
            executable_name,
            code
        );
    }
    else {
        auto_core_init.log_debug(
            "{} exited with code {}",
            executable_name,
            code
        );
    }
    return code;
}

/**
 * Deletes `config/auto_core.ini` if this initialization run created it and
 * the sequence does not finish. A file that already existed stays in place.
 */
struct SentinelGuard {
    std::filesystem::path path;
    bool armed = false;

    SentinelGuard(std::filesystem::path ini_path, const bool arm)
        : path(std::move(ini_path)),
          armed(arm) {
    }

    SentinelGuard(const SentinelGuard&) = delete;
    SentinelGuard& operator=(const SentinelGuard&) = delete;

    ~SentinelGuard() {
        if (!armed) {
            return;
        }
        std::error_code error;
        const bool removed = std::filesystem::remove(path, error);
        if (error) {
            auto_core_init.print(
                "Failed to remove {}: {}",
                path.string(),
                error.message()
            );
            return;
        }
        if (removed) {
            auto_core_init.print(
                "Removed {} because initialization did not complete.",
                path.string()
            );
        }
    }

    void release() noexcept {
        armed = false;
    }
};

[[nodiscard]]
std::string_view mode_name(const sequence::InitializationMode mode) {
    switch (mode) {
    case sequence::InitializationMode::seed:
        return "defaults";
    case sequence::InitializationMode::interactive:
        return "interactive";
    case sequence::InitializationMode::seeded_interactive:
        return "seeded interactive";
    }
    return "defaults";
}

[[nodiscard]]
std::optional<sequence::ComponentLaunch> prompt_components() {
    while (true) {
        std::cout
            << '\n'
            << "Components\n"
            << "\n"
            << "1. Use defaults for components\n"
            << "2. Configure components\n"
            << "3. Skip components\n"
            << "\n"
            << "Choice [1]: ";
        const auto line = cfg::read_line();
        if (!line) {
            return std::nullopt;
        }
        if (line->empty() || *line == "1") {
            return sequence::ComponentLaunch::seed;
        }
        if (*line == "2") {
            return sequence::ComponentLaunch::configure;
        }
        if (*line == "3") {
            return sequence::ComponentLaunch::skip;
        }
        std::cout << "Enter 1, 2, or 3.\n";
    }
}

[[nodiscard]]
int run_sequence(
    const sequence::InitializationMode mode,
    const bool prompt_for_components,
    const bool report_logging_initialized
) {
    if (report_logging_initialized) {
        ac::config::components_request::report_configuration_initialized(
            auto_core_init,
            "Logging"
        );
    }
    auto_core_init.log_debug(
        "Initialization mode selected: {}",
        mode_name(mode)
    );
    const auto ini = ac::paths::config_directory() / "auto_core.ini";
    std::error_code exists_error;
    const bool already_present = std::filesystem::exists(ini, exists_error);
    SentinelGuard sentinel {
        ini,
        !exists_error && !already_present
    };

    for (const auto& step : sequence::initialization_steps) {
        std::wstring argument_storage;
        std::wstring_view arguments;
        if (step.kind == sequence::OwnerKind::components) {
            auto launch = sequence::ComponentLaunch::seed;
            if (prompt_for_components) {
                const auto choice = prompt_components();
                if (!choice) {
                    auto_core_init.print(
                        "Component initialization cancelled."
                    );
                    return 1;
                }
                launch = *choice;
            }
            if (launch == sequence::ComponentLaunch::skip) {
                auto_core_init.print("Component initialization skipped.");
                continue;
            }
            arguments = sequence::component_arguments(launch);
        }
        else {
            const auto mode_arguments = sequence::arguments_for(step.kind, mode);
            if (!step.route.empty()) {
                argument_storage = step.route;
                if (!mode_arguments.empty()) {
                    argument_storage += L' ';
                    argument_storage += mode_arguments;
                }
                arguments = argument_storage;
            }
            else {
                arguments = mode_arguments;
            }
        }
        if (step.route == L"--config --area auto-core") {
            auto_core_init.log_debug("Starting auto_core.ini creation");
        }
        const int code = run_owner(step.executable, arguments);
        if (code != 0) {
            auto_core_init.log_debug("Required child failed");
            return 1;
        }
    }
    sentinel.release();
    auto_core_init.print("Configuration initialized successfully.");
    return 0;
}

void print_menu() {
    std::cout
        << "Auto Core Configuration\n"
        << "\n"
        << "1. Use defaults\n"
        << "2. Configure\n"
        << "\n"
        << "Choice [1]: ";
}

[[nodiscard]]
bool logging_ini_exists() {
    std::error_code error;
    return std::filesystem::exists(
        ac::paths::config_directory() / "logging_policy.ini",
        error
    ) && !error;
}

[[nodiscard]]
int launch_logging_config(const std::wstring_view arguments) {
    const auto path =
        ac::paths::bin_directory() / boot::logging_config_executable;
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error || !present) {
        std::cout << boot::logging_config_executable << " was not found.\n";
        return 1;
    }
    return cfg::run_config_exe(
        boot::logging_config_executable,
        std::wstring {arguments}
    );
}

[[nodiscard]]
std::optional<bool> prompt_auto_core_logging() {
    while (true) {
        std::cout << "Enable Auto Core logging? [Y/n]: ";
        std::cout.flush();
        const auto line = cfg::read_line();
        if (!line) {
            return std::nullopt;
        }
        if (line->empty() || *line == "y" || *line == "Y") {
            return true;
        }
        if (*line == "n" || *line == "N") {
            return false;
        }
        std::cout << "Enter Y or N.\n";
    }
}

/**
 * Persists disable_all, then records the final log line from the cached
 * enabled policy and stops further logging in this process. A failed update
 * leaves that cache enabled.
 */
void disable_auto_core_logging() {
    const int code = launch_logging_config(
        boot::config_arguments(boot::ConfigArguments::disable)
    );
    if (code != 0) {
        std::cerr
            << "Failed to update config/logging_policy.ini. "
            << "Logging remains enabled.\n";
        return;
    }
    auto_core_init.log_info("Auto Core logging has been disabled.");
    ac::logging::config::suppress();
}

[[nodiscard]]
int establish_logging(bool& created_policy) {
    created_policy = !logging_ini_exists();
    if (created_policy) {
        const auto outcome = boot::run_logging_config(
            boot::ConfigArguments::seed,
            [](const boot::ConfigArguments arguments) {
                return launch_logging_config(boot::config_arguments(arguments));
            },
            [] { return logging_ini_exists(); },
            [] { ac::logging::config::resolve(); }
        );
        if (outcome.code != 0 || !outcome.logging_started) {
            if (!logging_ini_exists()) {
                std::cerr << "config/logging_policy.ini was not created.\n";
            }
            return 1;
        }
    }
    else {
        ac::logging::config::resolve();
    }
    auto_core_init.log_info("Auto Core initialization");
    return 0;
}

} // namespace

int run_initialization_program(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto mode = boot::init_mode(launch->init, launch->seed);
    if (mode != boot::InitMode::seed) {
        std::cout << "Auto Core Initialization\n";
    }
    bool created_policy = false;
    if (establish_logging(created_policy) != 0) {
        return 1;
    }
    auto_core_init.log_info("auto_core_settings.exe --initialize started");
    std::error_code ini_error;
    const bool ini_present = std::filesystem::exists(
        ac::paths::config_directory() / "auto_core.ini",
        ini_error
    );
    if (!ini_error && !ini_present) {
        auto_core_init.log_debug(
            "config/auto_core.ini is missing. Running auto_core_settings.exe --initialize."
        );
    }

    if (mode == boot::InitMode::seed) {
        return run_sequence(
            sequence::InitializationMode::seed,
            false,
            created_policy
        );
    }
    if (mode == boot::InitMode::interactive_seed) {
        return run_sequence(
            sequence::InitializationMode::seeded_interactive,
            true,
            created_policy
        );
    }

    while (true) {
        print_menu();
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        if (line->empty() || *line == "1") {
            return run_sequence(
                sequence::InitializationMode::seed,
                true,
                created_policy
            );
        }
        if (*line == "2") {
            if (created_policy) {
                const auto enabled = prompt_auto_core_logging();
                if (!enabled) {
                    return 1;
                }
                if (!*enabled) {
                    disable_auto_core_logging();
                }
            }
            return run_sequence(
                sequence::InitializationMode::interactive,
                true,
                created_policy
            );
        }
        std::cout << "Enter 1 or 2.\n";
    }
}
