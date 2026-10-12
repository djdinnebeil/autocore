/**
 * \file logging_bootstrap.ixx
 * \brief Selects and launches logging_config.exe before normal logging.
 *
 * This module does not write logging.ini and does not run the Auto Core
 * menu or the owner sequence. Console streams are the only I/O used while
 * the file is missing.
 */
export module logging_bootstrap;

import std;

export namespace ac::main::logging_bootstrap {

    enum class InitMode {
        interactive,
        seed,
        interactive_seed
    };

    enum class ConfigArguments {
        preserve,
        seed,
        initialize,
        disable
    };

    struct Selection {
        bool cancelled {false};
        ConfigArguments arguments {ConfigArguments::preserve};
    };

    struct Outcome {
        int code {0};
        bool logging_started {false};
    };

    inline constexpr std::string_view logging_config_executable {
        "logging_config.exe"
    };

    [[nodiscard]]
    constexpr InitMode init_mode(
        const bool init_requested,
        const bool seed_requested
    ) noexcept {
        if (seed_requested && init_requested) {
            return InitMode::interactive_seed;
        }
        if (seed_requested) {
            return InitMode::seed;
        }
        return InitMode::interactive;
    }

    [[nodiscard]]
    constexpr std::wstring_view config_arguments(
        const ConfigArguments arguments
    ) noexcept {
        switch (arguments) {
        case ConfigArguments::seed:
            return L"--seed";
        case ConfigArguments::initialize:
            return L"--init";
        case ConfigArguments::disable:
            return L"--disable";
        case ConfigArguments::preserve:
            return L"";
        }
        return L"";
    }

    [[nodiscard]]
    Selection select_logging_config(
        InitMode mode,
        bool logging_ini_exists,
        std::istream& input,
        std::ostream& output
    );

    /**
     * Launches logging_config.exe when a write is required.
     *
     * `start_logging` runs only after the file is present. A failed launch
     * or a missing file leaves logging unstarted.
     */
    template <typename Launch, typename Exists, typename StartLogging>
    [[nodiscard]]
    Outcome run_logging_config(
        const ConfigArguments arguments,
        Launch&& launch,
        Exists&& exists,
        StartLogging&& start_logging
    ) {
        Outcome outcome;
        if (arguments != ConfigArguments::preserve) {
            outcome.code = launch(arguments);
            if (outcome.code != 0) {
                return outcome;
            }
        }
        if (!exists()) {
            outcome.code = 1;
            return outcome;
        }
        start_logging();
        outcome.logging_started = true;
        return outcome;
    }

} // namespace ac::main::logging_bootstrap

namespace {

    [[nodiscard]]
    std::optional<std::string> read_trimmed(std::istream& input) {
        std::string line;
        if (!std::getline(input, line)) {
            return std::nullopt;
        }
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos) {
            return std::string {};
        }
        const auto last = line.find_last_not_of(" \t\r");
        return line.substr(first, last - first + 1);
    }

    [[nodiscard]]
    std::optional<bool> prompt_enable(
        std::istream& input,
        std::ostream& output
    ) {
        while (true) {
            output << "Enable logging? [Y/n]: ";
            output.flush();
            const auto answer = read_trimmed(input);
            if (!answer) {
                return std::nullopt;
            }
            if (answer->empty() || *answer == "y" || *answer == "Y") {
                return true;
            }
            if (*answer == "n" || *answer == "N") {
                return false;
            }
            output << "Enter Y or N.\n";
        }
    }

    [[nodiscard]]
    std::optional<bool> prompt_configure(
        std::istream& input,
        std::ostream& output
    ) {
        while (true) {
            output
                << "Logging\n"
                << "\n"
                << "1. Use defaults\n"
                << "2. Configure\n"
                << "\n"
                << "Choice [1]: ";
            output.flush();
            const auto answer = read_trimmed(input);
            if (!answer) {
                return std::nullopt;
            }
            if (answer->empty() || *answer == "1") {
                return false;
            }
            if (*answer == "2") {
                return true;
            }
            output << "Enter 1 or 2.\n";
        }
    }

} // namespace

ac::main::logging_bootstrap::Selection
ac::main::logging_bootstrap::select_logging_config(
    const ac::main::logging_bootstrap::InitMode mode,
    const bool logging_ini_exists,
    std::istream& input,
    std::ostream& output
) {
    using ac::main::logging_bootstrap::ConfigArguments;
    using ac::main::logging_bootstrap::InitMode;
    using ac::main::logging_bootstrap::Selection;

    if (logging_ini_exists || mode == InitMode::seed) {
        return Selection {
            .cancelled = false,
            .arguments = logging_ini_exists
                ? ConfigArguments::preserve
                : ConfigArguments::seed
        };
    }

    const auto enabled = prompt_enable(input, output);
    if (!enabled) {
        return Selection {.cancelled = true};
    }
    if (!*enabled) {
        return Selection {
            .cancelled = false,
            .arguments = ConfigArguments::disable
        };
    }
    if (mode == InitMode::interactive_seed) {
        return Selection {
            .cancelled = false,
            .arguments = ConfigArguments::seed
        };
    }

    const auto configure = prompt_configure(input, output);
    if (!configure) {
        return Selection {.cancelled = true};
    }
    return Selection {
        .cancelled = false,
        .arguments = *configure
            ? ConfigArguments::initialize
            : ConfigArguments::seed
    };
}
