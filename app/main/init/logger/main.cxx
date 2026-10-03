/**
 * \file main.cxx
 * \brief Logger first-run orchestration. Does not write configuration files.
 *
 * Use defaults seeds logging.ini and logger.ini. Configure runs each owner's
 * interactive initialization. Disable logging asks logging_config.exe to
 * write disable_all = on and seeds logger.ini.
 */
import std;
import auto_core.core.component;
import auto_core.core.encoding;
import auto_core.core.paths;
import auto_core.main.config_support;
import logger_init_detail;

import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace menu = ac::main::logger_init;

namespace {

ac::Component logger_init {"logger_init"};

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
        logger_init.log_print("{} was not found.", executable_name);
        std::cout << executable_name << " was not found.\n";
        return 1;
    }

    logger_init.log_print("Launching {}", command);
    const int code = cfg::run_config_exe(
        executable_name,
        std::wstring {arguments}
    );
    logger_init.log_print(
        "{} exited with code {}",
        executable_name,
        code
    );
    return code;
}

[[nodiscard]]
bool logging_ini_exists() {
    std::error_code error;
    return std::filesystem::exists(
        ac::paths::config_directory() / "logging.ini",
        error
    ) && !error;
}

[[nodiscard]]
int run_choice(const menu::Choice choice) {
    for (const auto& launch : menu::launches_for(choice)) {
        if (run_owner(launch.executable, launch.arguments) != 0) {
            logger_init.log_print("Required child failed");
            return 1;
        }
    }
    if (!logging_ini_exists()) {
        logger_init.log_print("Required child failed");
        return 1;
    }
    logger_init.log_print("Logger initialization completed successfully");
    return 0;
}

} // namespace

int main() {
    ac::shell::set_process_app_user_model_id();
    logger_init.log_main("logger_init.exe started");

    const auto choice = menu::prompt_menu(std::cin, std::cout);
    if (!choice) {
        logger_init.log_print("Logger initialization cancelled");
        return 1;
    }
    return run_choice(*choice);
}
