/**
 * \file main.cxx
 * \brief First-run orchestration for Auto Core. Does not write configuration files.
 *
 * When this run creates `config/auto_core.ini`, a failed or cancelled required
 * step removes that file before exit. Logger and Components initialization
 * are required stages with their own menus.
 */
import std;
import auto_core.core.component;
import auto_core.core.encoding;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core_initialization;

import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;

namespace {

ac::Component auto_core_init {"auto_core_init"};

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
        auto_core_init.log_print("{} was not found.", executable_name);
        std::cout << executable_name << " was not found.\n";
        return 1;
    }

    auto_core_init.log_print("Launching {}", command);
    const int code = cfg::run_config_exe(
        executable_name,
        std::wstring {arguments}
    );
    auto_core_init.log_print(
        "{} exited with code {}",
        executable_name,
        code
    );
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
            auto_core_init.log_print(
                "Failed to remove {}: {}",
                path.string(),
                error.message()
            );
            return;
        }
        if (removed) {
            auto_core_init.log_print(
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
int run_sequence(const bool defaults) {
    auto_core_init.log_print(
        "Initialization mode selected: {}",
        defaults ? "defaults" : "interactive"
    );
    const auto ini = ac::paths::config_directory() / "auto_core.ini";
    std::error_code exists_error;
    const bool already_present = std::filesystem::exists(ini, exists_error);
    SentinelGuard sentinel {
        ini,
        !exists_error && !already_present
    };

    namespace sequence = ac::main::init;
    for (const auto& step : sequence::initialization_steps) {
        const auto arguments = sequence::arguments_for(step.kind, defaults);
        if (step.kind == sequence::OwnerKind::auto_core) {
            auto_core_init.log_print("Starting auto_core.ini creation");
        }
        const int code = run_owner(step.executable, arguments);
        if (code != 0) {
            auto_core_init.log_print("Required child failed");
            return 1;
        }
        if (step.kind == sequence::OwnerKind::logger) {
            std::error_code logging_error;
            const auto logging_ini =
                ac::paths::config_directory() / "logging.ini";
            const bool logging_present =
                std::filesystem::exists(logging_ini, logging_error);
            if (logging_error || !logging_present) {
                auto_core_init.log_print("Required child failed");
                return 1;
            }
        }
    }
    sentinel.release();
    auto_core_init.log_print("Initialization completed successfully");
    return 0;
}

void print_menu() {
    std::cout
        << "Auto Core\n"
        << "\n"
        << "1. Use defaults\n"
        << "2. Configure\n"
        << "Choice: ";
}

} // namespace

int main() {
    ac::shell::set_process_app_user_model_id();
    auto_core_init.log_main("auto_core_init.exe started");

    while (true) {
        print_menu();
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        if (*line == "1") {
            return run_sequence(true);
        }
        if (*line == "2") {
            return run_sequence(false);
        }
        std::cout << "Enter 1 or 2.\n";
    }
}
