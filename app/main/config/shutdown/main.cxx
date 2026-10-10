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

ac::Component shutdown_config {"shutdown_config"};

constexpr std::string_view prompt_values[] {"popup", "console"};

const menu::Setting settings[] {
    {
        .key = "delayed_shutdown_prompt",
        .display_name = "Delayed shutdown prompt",
        .summary = "How Main waits when a shutdown needs a response.",
        .default_value = defaults::shutdown_prompt,
        .choices = prompt_values,
    },
    {
        .key = "shutdown_timeout_ms",
        .display_name = "Shutdown timeout",
        .summary = "Milliseconds Main waits for a generic child to exit.",
        .default_value = "2000",
        .constraint = "Enter a non-negative number of milliseconds.",
    },
};

struct ShutdownValues {
    std::string prompt {defaults::shutdown_prompt};
    unsigned timeout_ms {defaults::shutdown_timeout_ms};
};

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "shutdown.ini";
}

[[nodiscard]]
ShutdownValues current_values() {
    ShutdownValues values;
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return values;
    }
    if (const auto prompt = document->find("shutdown", "delayed_shutdown_prompt");
        prompt && (*prompt == "popup" || *prompt == "console")) {
        values.prompt = std::string {*prompt};
    }
    if (const auto timeout = document->find("shutdown", "shutdown_timeout_ms")) {
        unsigned milliseconds = 0;
        const auto parsed = std::from_chars(
            timeout->data(),
            timeout->data() + timeout->size(),
            milliseconds
        );
        if (parsed.ec == std::errc {} &&
            parsed.ptr == timeout->data() + timeout->size()) {
            values.timeout_ms = milliseconds;
        }
    }
    return values;
}

[[nodiscard]]
bool write_values(const ShutdownValues& values) {
    return cfg::write_bytes(
        ini_path(),
        menu::with_header(
            settings,
            defaults::ini_for_shutdown(values.prompt, values.timeout_ms)
        )
    );
}

[[nodiscard]]
std::optional<unsigned> parse_timeout(const std::string_view text) {
    unsigned milliseconds = 0;
    const auto parsed = std::from_chars(
        text.data(),
        text.data() + text.size(),
        milliseconds
    );
    if (parsed.ec != std::errc {} || parsed.ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return milliseconds;
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    const auto values = current_values();
    if (setting.key == "delayed_shutdown_prompt") {
        return values.prompt;
    }
    return std::to_string(values.timeout_ms);
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    auto values = current_values();
    if (setting.key == "delayed_shutdown_prompt") {
        values.prompt = std::string {value};
    }
    else {
        const auto timeout = parse_timeout(value);
        if (!timeout) {
            return menu::ApplyResult::invalid;
        }
        values.timeout_ms = *timeout;
    }
    if (!write_values(values)) {
        shutdown_config.log_print("Failed to write {}.", ini_path().string());
        return menu::ApplyResult::failed;
    }
    shutdown_config.log_print("Wrote {}", ini_path().string());
    return menu::ApplyResult::stored;
}

[[nodiscard]]
int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Shutdown Configuration",
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
    if (!write_values({})) {
        shutdown_config.log_print("Failed to write {}.", ini_path().string());
        return 1;
    }
    shutdown_config.log_print("Wrote {}", ini_path().string());
    return 0;
}

[[nodiscard]]
int initialize_missing() {
    if (write_missing_defaults() != 0) {
        return 1;
    }
    const auto offer = menu::offer_configuration(
        "Shutdown Configuration",
        settings,
        current_text,
        std::cin,
        std::cout
    );
    if (offer == menu::OfferResult::failed) {
        return 1;
    }
    if (offer == menu::OfferResult::configure) {
        return run_configuration_menu();
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    shutdown_config.log_main("shutdown_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    std::error_code error;
    const bool present = std::filesystem::exists(ini_path(), error);
    if (error) {
        shutdown_config.log_print(
            "Failed to inspect {}",
            ini_path().string()
        );
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(shutdown_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(shutdown_config, "config/shutdown.ini");
            return 0;
        }
        req::log_writing_defaults(shutdown_config);
        if (write_missing_defaults() != 0) {
            return 1;
        }
        req::log_configuration_initialized(shutdown_config);
        return 0;
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(
                shutdown_config,
                "config/shutdown.ini"
            );
            return 0;
        }
        req::log_configuration_missing(shutdown_config);
        req::log_writing_defaults(shutdown_config);
        if (initialize_missing() != 0) {
            return 1;
        }
        req::log_configuration_initialized(shutdown_config);
        return 0;
    }

    if (!present) {
        req::log_configuration_missing(shutdown_config);
        req::log_writing_defaults(shutdown_config);
        if (write_missing_defaults() != 0) {
            return 1;
        }
    }
    return run_configuration_menu();
}
