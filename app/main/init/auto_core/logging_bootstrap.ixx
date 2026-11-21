/**
 * \file logging_bootstrap.ixx
 * \brief Launches the logging-policy route before normal logging.
 *
 * This module does not write logging_policy.ini and does not prompt.
 * The Main menu decides whether a policy created for this run stays enabled.
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
        disable
    };

    struct Outcome {
        int code {0};
        bool logging_started {false};
    };

    inline constexpr std::string_view logging_config_executable {
        "auto_core_settings.exe"
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
            return L"--config --area logging-policy --seed";
        case ConfigArguments::disable:
            return L"--config --area logging-policy --disable";
        case ConfigArguments::preserve:
            return L"--config --area logging-policy";
        }
        return L"";
    }

    /**
     * Launches the logging-policy route when a write is required.
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
