/**
 * \file main.cxx
 * \brief Coordinates component configuration executables. Does not write files.
 *
 * `--init` asks each component whether to seed, configure, or skip.
 * `--seed` and `--init --seed` launch every component config with
 * `--init --seed` and do not prompt.
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
        components_init.print(
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
        components_init.print(
            "Component config executable not found: {}",
            helper
        );
        components_init.log_debug("Skipping component: {}", name);
        return {.skipped = true, .code = 0};
    }

    components_init.log_debug("Launching {}", command);
    const int code = cfg::run_config_exe(helper, std::wstring {arguments});
    if (code != 0) {
        components_init.print("{} exited with code {}.", helper, code);
    }
    else {
        components_init.log_debug("{} exited with code {}", helper, code);
    }
    return {.skipped = false, .code = code};
}

void log_summary(
    const int discovered,
    const int succeeded,
    const int skipped,
    const int failed
) {
    components_init.print("Component initialization complete.");
    components_init.log_debug("Components discovered: {}", discovered);
    components_init.log_debug("Components succeeded: {}", succeeded);
    components_init.log_debug("Components skipped: {}", skipped);
    components_init.log_debug("Components failed: {}", failed);
}

[[nodiscard]]
int use_defaults(const std::vector<std::string>& names) {
    int succeeded = 0;
    int skipped = 0;
    int failed = 0;
    for (const auto& name : names) {
        components_init.log_debug("Discovered component: {}", name);
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

enum class ComponentChoice {
    defaults,
    configure,
    skip
};

[[nodiscard]]
std::string prompt_title(const std::string_view name) {
    if (name == "itunes") {
        return "iTunes";
    }
    std::string title {name};
    if (!title.empty() && title.front() >= 'a' && title.front() <= 'z') {
        title.front() = static_cast<char>(title.front() - 'a' + 'A');
    }
    return title;
}

[[nodiscard]]
bool config_executable_present(const std::string_view name) {
    const auto helper = std::string {name} + "_config.exe";
    std::error_code error;
    const bool present = std::filesystem::exists(
        ac::paths::bin_directory() / helper,
        error
    );
    return !error && present;
}

[[nodiscard]]
std::optional<ComponentChoice> prompt_component(const std::string_view name) {
    const auto title = prompt_title(name);
    while (true) {
        std::cout
            << '\n'
            << title << "\n"
            << "\n"
            << "1. Use defaults\n"
            << "2. Configure\n"
            << "3. Skip\n"
            << "\n"
            << "Choice [1]: ";
        const auto line = cfg::read_line();
        if (!line) {
            return std::nullopt;
        }
        if (line->empty() || *line == "1") {
            return ComponentChoice::defaults;
        }
        if (*line == "2") {
            return ComponentChoice::configure;
        }
        if (*line == "3") {
            return ComponentChoice::skip;
        }
        std::cout << "Enter 1, 2, or 3.\n";
    }
}

[[nodiscard]]
int prompt_each(const std::vector<std::string>& names) {
    int succeeded = 0;
    int skipped = 0;
    int failed = 0;
    for (const auto& name : names) {
        components_init.log_debug("Discovered component: {}", name);
        if (!config_executable_present(name)) {
            components_init.print(
                "Component config executable not found: {}_config.exe",
                name
            );
            components_init.log_debug("Skipping component: {}", name);
            ++skipped;
            continue;
        }

        const auto choice = prompt_component(name);
        if (!choice) {
            components_init.print(
                "Component initialization cancelled."
            );
            log_summary(
                static_cast<int>(names.size()),
                succeeded,
                skipped,
                failed
            );
            return 2;
        }
        if (*choice == ComponentChoice::skip) {
            components_init.log_debug("Skipping component: {}", name);
            ++skipped;
            continue;
        }

        const auto arguments = *choice == ComponentChoice::configure
            ? L"--init"
            : L"--init --seed";
        const auto outcome = launch_config(name, arguments);
        if (outcome.skipped) {
            ++skipped;
            continue;
        }
        if (outcome.code != 0) {
            ++failed;
            components_init.log_debug(
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

struct LaunchMode {
    bool init {false};
    bool seed {false};
};

[[nodiscard]]
std::optional<LaunchMode> parse_launch(const int argc, char* argv[]) {
    LaunchMode mode;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument {argv[i]};
        if (argument == "--init" || argument == "--initialize") {
            mode.init = true;
            continue;
        }
        if (argument == "--seed") {
            mode.seed = true;
            continue;
        }
        std::cerr << "Unknown argument: " << argument << '\n';
        return std::nullopt;
    }
    return mode;
}

[[nodiscard]]
int run_discovered(const bool interactive) {
    components_init.log_debug(
        "Component initialization mode: {}",
        interactive ? "interactive" : "defaults"
    );
    auto names = discover_components();
    if (!names) {
        return 1;
    }
    if (names->empty()) {
        components_init.print("No components were discovered.");
        components_init.print("Component initialization complete.");
        components_init.log_debug("Components discovered: 0");
        components_init.log_debug("Components succeeded: 0");
        components_init.log_debug("Components skipped: 0");
        components_init.log_debug("Components failed: 0");
        return 0;
    }
    list::prioritize_taskbar(*names);
    if (interactive) {
        return prompt_each(*names);
    }
    return use_defaults(*names);
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    components_init.log_info("components_init.exe started");

    const auto launch = parse_launch(argc, argv);
    if (!launch) {
        return 1;
    }
    if (launch->seed) {
        return run_discovered(false);
    }
    return run_discovered(true);
}
