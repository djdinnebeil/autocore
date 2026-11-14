import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import auto_core.main.installed_version;
import auto_core.version;
import components_editor_request;
import config_menu;

import <iostream>;
import auto_core.core.shell;

#include "config_areas.hpp"

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;
namespace menu = ac::config_menu;

namespace {

ac::Component auto_core_config {"auto_core_config"};

constexpr std::string_view on_off[] {"on", "off"};
constexpr std::string_view version_value[] {ac::version::auto_core_version};

const menu::Setting menu_settings[] {
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging for auto_core.exe.",
        .default_value = "on",
        .choices = on_off,
    },
    {
        .key = "check_for_updates",
        .display_name = "Check for updates",
        .summary = "Checks GitHub for a newer Auto Core version at startup.",
        .default_value = "on",
        .choices = on_off,
    },
};

struct AutoCoreFlags {
    bool logging;
    bool check_for_updates;
};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "auto_core.ini";
}

[[nodiscard]]
bool enabled_value(
    const std::optional<std::string_view> value,
    const bool fallback
) {
    if (!value) {
        return fallback;
    }
    if (*value == "off") {
        return false;
    }
    if (*value == "on") {
        return true;
    }
    return fallback;
}

[[nodiscard]]
AutoCoreFlags current_flags() {
    AutoCoreFlags flags {
        .logging = ac::logging::config::component_logging_default(),
        .check_for_updates = defaults::auto_core_check_for_updates,
    };
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return flags;
    }
    flags.logging = enabled_value(
        document->find("auto_core", "logging"),
        flags.logging
    );
    flags.check_for_updates = enabled_value(
        document->find("auto_core", "check_for_updates"),
        flags.check_for_updates
    );
    return flags;
}

[[nodiscard]]
bool write_settings(const AutoCoreFlags& flags) {
    const menu::Setting header[] {
        {
            .key = "version",
            .choices = version_value,
        },
        {
            .key = "check_for_updates",
            .choices = on_off,
        },
        {
            .key = "logging",
            .choices = on_off,
        },
    };
    return cfg::write_bytes(
        ini_path(),
        menu::with_header(
            header,
            defaults::ini_for_auto_core(
                ac::version::auto_core_version,
                flags.logging,
                flags.check_for_updates
            )
        )
    );
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    const auto flags = current_flags();
    const bool enabled = setting.key == "check_for_updates"
        ? flags.check_for_updates
        : flags.logging;
    return enabled ? "on" : "off";
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    AutoCoreFlags flags = current_flags();
    const bool enabled = value == "on";
    if (setting.key == "check_for_updates") {
        flags.check_for_updates = enabled;
    }
    else {
        flags.logging = enabled;
    }
    if (!write_settings(flags)) {
        auto_core_config.print("Failed to write {}.", ini_path().string());
        return menu::ApplyResult::failed;
    }
    auto_core_config.log_debug("Wrote {}", ini_path().string());
    return menu::ApplyResult::stored;
}

[[nodiscard]]
int configure_settings() {
    const auto result = menu::run_menu(
        "Auto Core Configuration",
        menu_settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
}

[[nodiscard]]
int create_auto_core_ini(const bool prompt, const bool walk_settings = false) {
    const auto path = ini_path();
    if (!prompt) {
        if (!write_settings({
                .logging = defaults::auto_core_logging,
                .check_for_updates = defaults::auto_core_check_for_updates,
            })) {
            auto_core_config.print("Failed to write {}.", path.string());
            return 1;
        }
        auto_core_config.log_debug("Wrote {}", path.string());
        return 0;
    }

    if (!write_settings({
            .logging = defaults::auto_core_logging,
            .check_for_updates = defaults::auto_core_check_for_updates,
        })) {
        auto_core_config.print("Failed to write {}.", path.string());
        return 1;
    }
    auto_core_config.log_debug("Wrote {}", path.string());
    if (walk_settings) {
        const auto result = menu::prompt_settings(
            menu_settings,
            current_text,
            apply_setting,
            std::cin,
            std::cout,
            "Runtime Configuration"
        );
        return result.ok ? 0 : 1;
    }
    const auto offer = menu::offer_configuration(
        "Auto Core Configuration",
        menu_settings,
        current_text,
        std::cin,
        std::cout
    );
    if (offer == menu::OfferResult::failed) {
        return 1;
    }
    if (offer == menu::OfferResult::configure) {
        return configure_settings();
    }
    return 0;
}

} // namespace

int run_auto_core_config(int argc, char* argv[]) {
    auto_core_config.log_info("Auto Core configuration opened");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ini_path();
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        auto_core_config.print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(auto_core_config, *launch);
    if (present && !ac::main::synchronize_installed_version()) {
        auto_core_config.print("Failed to write {}.", path.string());
        return 1;
    }
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(auto_core_config, "config/auto_core.ini");
            return req::finish_configuration_initialization(
                auto_core_config,
                0,
                "Runtime"
            );
        }
        req::log_writing_defaults(auto_core_config);
        if (create_auto_core_ini(false) != 0) {
            return 1;
        }
        req::log_configuration_initialized(auto_core_config);
        return req::finish_configuration_initialization(
            auto_core_config,
            0,
            "Runtime"
        );
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(
                auto_core_config,
                "config/auto_core.ini"
            );
            return req::finish_configuration_initialization(
                auto_core_config,
                0,
                "Runtime"
            );
        }
        req::log_configuration_missing(auto_core_config);
        return req::finish_configuration_initialization(
            auto_core_config,
            create_auto_core_ini(true, true),
            "Runtime"
        );
    }

    if (present) {
        return configure_settings();
    }
    return create_auto_core_ini(true);
}

namespace {

struct ConfigArea {
    std::string_view route;
    std::string_view display_name;
    int (*run)(int, char*[]);
};

constexpr ConfigArea config_areas[] {
    {"auto-core", "Auto Core", run_auto_core_config},
    {"components-policy", "Components Policy", run_components_policy_config},
    {"crash-recovery", "Crash Recovery", run_crash_recovery_config},
    {"logging-policy", "Logging Policy", run_logging_policy_config},
    {"shutdown", "Shutdown", run_shutdown_config},
    {"startup", "Startup", run_startup_config},
    {"warnings", "Warnings", run_warnings_config},
};

[[nodiscard]]
const ConfigArea* find_area(const std::string_view route) {
    const auto found = std::ranges::find(config_areas, route, &ConfigArea::route);
    return found == std::end(config_areas) ? nullptr : &*found;
}

[[nodiscard]]
int run_area(const ConfigArea& area, const std::span<char*> arguments) {
    const int code = area.run(
        static_cast<int>(arguments.size()),
        arguments.data()
    );
    if (code != 0) {
        auto_core_config.print(
            "{} configuration did not complete successfully.",
            area.display_name
        );
    }
    return code;
}

void print_area_menu() {
    std::cout
        << "\nAuto Core Configuration\n"
        << "\n"
        << "1. Auto Core\n"
        << "2. Components Policy\n"
        << "3. Crash Recovery\n"
        << "4. Logging Policy\n"
        << "5. Shutdown\n"
        << "6. Startup\n"
        << "7. Warnings\n"
        << "0. Exit\n"
        << "Choice: ";
}

[[nodiscard]]
int run_area_menu(char* executable) {
    std::array<char*, 1> arguments {executable};
    while (true) {
        print_area_menu();
        const auto line = cfg::read_line();
        if (!line) {
            return 1;
        }
        if (*line == "0" || line->empty()) {
            return 0;
        }
        unsigned selection = 0;
        const auto parsed = std::from_chars(
            line->data(), line->data() + line->size(), selection
        );
        if (parsed.ec != std::errc {} ||
            parsed.ptr != line->data() + line->size() ||
            selection < 1 || selection > std::size(config_areas)) {
            std::cout << "Enter 0-7.\n";
            continue;
        }
        (void)run_area(config_areas[selection - 1], arguments);
    }
}

} // namespace

int run_configuration_program(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();

    if (argc == 1) {
        return run_area_menu(argv[0]);
    }

    if (std::string_view {argv[1]} != "--area") {
        // Preserve the retained executable's existing --seed/--init behavior.
        return run_auto_core_config(argc, argv);
    }
    if (argc < 3) {
        std::cerr << "Missing configuration area after --area.\n";
        return 1;
    }
    const auto* area = find_area(argv[2]);
    if (!area) {
        std::cerr << "Unknown configuration area: " << argv[2] << '\n';
        return 1;
    }

    std::vector<char*> arguments;
    arguments.reserve(static_cast<std::size_t>(argc - 1));
    arguments.push_back(argv[0]);
    arguments.insert(arguments.end(), argv + 3, argv + argc);
    return area->run(static_cast<int>(arguments.size()), arguments.data());
}
