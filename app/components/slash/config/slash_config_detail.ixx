/**
 * \file slash_config_detail.ixx
 * \brief Launch selection for Slash configuration.
 */
export module slash_config_detail;

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

} // namespace slash::config
