/**
 * \file main.cxx
 * \brief Coordinates component configuration executables. Does not write files.
 */
#include "../../../core/component/components_list_detail.hpp"

import std;
import auto_core.core.component;
import auto_core.core.encoding;
import auto_core.core.paths;
import auto_core.main.config_support;

import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace list = ac::config::components_list;

namespace {

ac::Component components_init {"components_init"};

[[nodiscard]]
std::optional<std::vector<std::string>> discover_components() {
    const auto bin = ac::paths::bin_directory();
    std::error_code error;
    const auto iterator = std::filesystem::directory_iterator(bin, error);
    if (error) {
        components_init.log_print(
            "Failed to inspect {}: {}",
            bin.string(),
            error.message()
        );
        return std::nullopt;
    }
    (void)iterator;
    return list::discover_ac_executables(bin);
}

struct LaunchOutcome {
    bool skipped {false};
    int code {0};
};

[[nodiscard]]
LaunchOutcome launch_config(
    const std::string_view name,
    const std::wstring_view arguments
) {
    const auto helper = std::string {name} + "_config.exe";
    std::string command = helper;
    if (!arguments.empty()) {
        command += ' ';
        command += ac::encoding::to_utf8(arguments);
    }
    const auto path = ac::paths::bin_directory() / helper;
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error || !present) {
        components_init.log_print(
            "Component config executable not found: {}",
            helper
        );
        components_init.log_print("Skipping component: {}", name);
        return {.skipped = true, .code = 0};
    }

    components_init.log_print("Launching {}", command);
    const int code = cfg::run_config_exe(helper, std::wstring {arguments});
    components_init.log_print("{} exited with code {}", helper, code);
    return {.skipped = false, .code = code};
}

void log_summary(
    const int discovered,
    const int succeeded,
    const int skipped,
    const int failed
) {
    components_init.log_print("Component initialization completed");
    components_init.log_print("Components discovered: {}", discovered);
    components_init.log_print("Components succeeded: {}", succeeded);
    components_init.log_print("Components skipped: {}", skipped);
    components_init.log_print("Components failed: {}", failed);
}

[[nodiscard]]
int use_defaults(const std::vector<std::string>& names) {
    int succeeded = 0;
    int skipped = 0;
    int failed = 0;
    for (const auto& name : names) {
        components_init.log_print("Discovered component: {}", name);
        const auto outcome = launch_config(name, L"--init --seed");
        if (outcome.skipped) {
            ++skipped;
        }
        else if (outcome.code == 0) {
            ++succeeded;
        }
        else {
            ++failed;
        }
    }
    log_summary(
        static_cast<int>(names.size()),
        succeeded,
        skipped,
        failed
    );
    return 0;
}

[[nodiscard]]
int configure_each(const std::vector<std::string>& names) {
    int succeeded = 0;
    int skipped = 0;
    int failed = 0;
    for (const auto& name : names) {
        components_init.log_print("Discovered component: {}", name);
        const auto outcome = launch_config(name, L"--init");
        if (outcome.skipped) {
            ++skipped;
            continue;
        }
        if (outcome.code != 0) {
            ++failed;
            components_init.log_print(
                "Component initialization stopped after a non-zero "
                "interactive result"
            );
            log_summary(
                static_cast<int>(names.size()),
                succeeded,
                skipped,
                failed
            );
            return 2;
        }
        ++succeeded;
    }
    log_summary(
        static_cast<int>(names.size()),
        succeeded,
        skipped,
        failed
    );
    return 0;
}

void print_menu() {
    std::cout
        << "\nComponent Initialization\n"
        << "\n"
        << "1. Use default settings for components\n"
        << "2. Configure settings for each component\n"
        << "3. Exit\n"
        << "Choice: ";
}

} // namespace

int main() {
    ac::shell::set_process_app_user_model_id();
    components_init.log_main("components_init.exe started");

    while (true) {
        print_menu();
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        if (*line == "3" || line->empty()) {
            components_init.log_print(
                "Component initialization cancelled by user"
            );
            return 2;
        }
        if (*line != "1" && *line != "2") {
            std::cout << "Enter 1, 2, or 3.\n";
            continue;
        }

        components_init.log_print(
            "Component initialization mode: {}",
            *line == "1" ? "defaults" : "interactive"
        );
        const auto names = discover_components();
        if (!names) {
            return 1;
        }
        if (names->empty()) {
            components_init.log_print("No components were discovered.");
            components_init.log_print("Component initialization completed");
            components_init.log_print("Components discovered: 0");
            components_init.log_print("Components succeeded: 0");
            components_init.log_print("Components skipped: 0");
            components_init.log_print("Components failed: 0");
            return 0;
        }
        if (*line == "1") {
            return use_defaults(*names);
        }
        return configure_each(*names);
    }
}
