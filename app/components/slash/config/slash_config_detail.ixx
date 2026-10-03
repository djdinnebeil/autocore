/**
 * \file slash_config_detail.ixx
 * \brief Launch selection and interactive prompts for Slash configuration.
 */
export module slash_config_detail;

import std;
import slash_defaults;

namespace {

    [[nodiscard]] std::string trim(const std::string_view value) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t\r\n");
        return std::string {value.substr(first, last - first + 1)};
    }

} // namespace

export namespace slash::config {

    enum class Action {
        configure,
        initialize,
        seed,
        skip_initialization,
        skip_seed
    };

    /** Seed takes precedence over interactive initialization. */
    [[nodiscard]] Action select_action(
        const bool configuration_exists,
        const bool init_requested,
        const bool seed_requested
    ) noexcept {
        if (seed_requested) {
            return configuration_exists ? Action::skip_seed : Action::seed;
        }
        if (init_requested) {
            return configuration_exists
                ? Action::skip_initialization
                : Action::initialize;
        }
        return configuration_exists ? Action::configure : Action::initialize;
    }

    [[nodiscard]] std::optional<std::string> prompt_mode(
        std::istream& input,
        std::ostream& output,
        const std::string_view current
    ) {
        while (true) {
            output << "Slash mode [" << current << "]: ";
            output.flush();

            std::string line;
            if (!std::getline(input, line)) {
                return std::nullopt;
            }
            const auto value = trim(line);
            if (value.empty()) {
                return std::string {current};
            }
            if (slash::defaults::is_mode(value)) {
                return value;
            }
            output << "Enter verbose, concise, or silent.\n";
        }
    }

    [[nodiscard]] std::optional<bool> prompt_logging(
        std::istream& input,
        std::ostream& output,
        const bool current
    ) {
        while (true) {
            output << "Enable logging [" << (current ? "on" : "off") << "]: ";
            output.flush();

            std::string line;
            if (!std::getline(input, line)) {
                return std::nullopt;
            }
            const auto value = trim(line);
            if (value.empty()) {
                return current;
            }
            if (value == "on") {
                return true;
            }
            if (value == "off") {
                return false;
            }
            output << "Enter on or off.\n";
        }
    }

} // namespace slash::config
