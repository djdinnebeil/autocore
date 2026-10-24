import std;
import auto_core.core.component;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.main.config_support;
import auto_core.main.defaults;
import components_editor_request;
import config_menu;

import <iostream>;
import auto_core.core.shell;

namespace cfg = ac::main::config;
namespace defaults = ac::main::defaults;
namespace menu = ac::config_menu;

namespace {

ac::Component startup_config {"startup_config"};

constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting settings[] {
    {
        .key = "show_day_of_week",
        .display_name = "Show day of week",
        .summary = "Display the current weekday when Auto Core starts.",
        .default_value = "on",
        .choices = on_off,
    },
    {
        .key = "show_task_list",
        .display_name = "Show task list",
        .summary = "Display Writer's task list when Auto Core starts.",
        .default_value = "on",
        .choices = on_off,
    },
};

struct StartupFlags {
    bool show_day_of_week {defaults::startup_show_day_of_week};
    bool show_task_list {defaults::startup_show_task_list};
};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "startup.ini";
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
std::optional<ac::ini::Document> read_document() {
    std::ifstream input(ini_path(), std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    const std::string contents {
        std::istreambuf_iterator<char> {input},
        std::istreambuf_iterator<char> {}
    };
    if (input.bad()) {
        return std::nullopt;
    }
    return ac::ini::parse("[startup]\n" + contents);
}

[[nodiscard]]
StartupFlags current_flags() {
    StartupFlags flags;
    const auto document = read_document();
    if (!document) {
        return flags;
    }
    flags.show_day_of_week = enabled_value(
        document->find("startup", "show_day_of_week"),
        defaults::startup_show_day_of_week
    );
    flags.show_task_list = enabled_value(
        document->find("startup", "show_task_list"),
        defaults::startup_show_task_list
    );
    return flags;
}

[[nodiscard]]
bool write_flags(const StartupFlags& flags) {
    return cfg::write_bytes(
        ini_path(),
        menu::with_header(
            settings,
            defaults::ini_for_startup(
                flags.show_day_of_week,
                flags.show_task_list
            )
        )
    );
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    const auto flags = current_flags();
    const bool enabled = setting.key == "show_day_of_week"
        ? flags.show_day_of_week
        : flags.show_task_list;
    return enabled ? "on" : "off";
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    StartupFlags flags = current_flags();
    const bool enabled = value == "on";
    if (setting.key == "show_day_of_week") {
        flags.show_day_of_week = enabled;
    }
    else {
        flags.show_task_list = enabled;
    }
    if (!write_flags(flags)) {
        startup_config.print("Failed to write {}.", ini_path().string());
        return menu::ApplyResult::failed;
    }
    startup_config.log_debug("Wrote {}", ini_path().string());
    return menu::ApplyResult::stored;
}

[[nodiscard]]
int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Startup Configuration",
        settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
}

[[nodiscard]]
int write_missing_defaults() {
    if (!write_flags({})) {
        startup_config.print("Failed to write {}.", ini_path().string());
        return 1;
    }
    startup_config.log_debug("Wrote {}", ini_path().string());
    return 0;
}

[[nodiscard]]
int initialize_missing() {
    if (write_missing_defaults() != 0) {
        return 1;
    }
    const auto result = menu::prompt_settings(
        settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout,
        "Startup Configuration"
    );
    return result.ok ? 0 : 1;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    startup_config.log_info("startup_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    std::error_code error;
    const bool present = std::filesystem::exists(ini_path(), error);
    if (error) {
        startup_config.print("Failed to inspect {}", ini_path().string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(startup_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(startup_config, "config/startup.ini");
            return req::finish_configuration_initialization(
                startup_config,
                0,
                "Startup"
            );
        }
        req::log_writing_defaults(startup_config);
        if (write_missing_defaults() != 0) {
            return 1;
        }
        req::log_configuration_initialized(startup_config);
        return req::finish_configuration_initialization(
            startup_config,
            0,
            "Startup"
        );
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(
                startup_config,
                "config/startup.ini"
            );
            return req::finish_configuration_initialization(
                startup_config,
                0,
                "Startup"
            );
        }
        req::log_configuration_missing(startup_config);
        req::log_writing_defaults(startup_config);
        if (initialize_missing() != 0) {
            return 1;
        }
        req::log_configuration_initialized(startup_config);
        return req::finish_configuration_initialization(
            startup_config,
            0,
            "Startup"
        );
    }

    if (!present) {
        req::log_configuration_missing(startup_config);
        req::log_writing_defaults(startup_config);
        if (write_missing_defaults() != 0) {
            return 1;
        }
    }
    return run_configuration_menu();
}
