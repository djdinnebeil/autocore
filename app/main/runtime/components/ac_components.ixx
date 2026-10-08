/**
 * \file ac_components.ixx
 * \brief Generic Main-side host for Auto Core boot-time components.
 *
 * A normal component is an executable that satisfies `ac.component.v1`.
 * Main must not know that component at compile time.
 */
export module auto_core.main.components;

export import command_registry;
import std;

export namespace ac::main::components {
    class Session;

    /**
     * \brief Starts enabled generic v1 components from `components.list`
     *        `[components]`.
     * \return An RAII session. `shutdown()` (from `close_program()`) stops
     *         children in reverse successful-start order. Names with a
     *         an embedded `AC_LAUNCH_DESCRIPTOR` are not session children.
     */
    [[nodiscard]] Session initialize();

    /**
     * \brief Stops successfully started generic components.
     *
     * Safe to call more than once. Failed or disabled children are skipped.
     * `allow_recovery_prompt` must be false for noninteractive shutdown paths.
     */
    void shutdown(bool allow_recovery_prompt = true);

    /** `true` when interactive shutdown recovery uses the existing console. */
    [[nodiscard]] bool console_shutdown_prompt_enabled();

    /** Shared deadline from `shutdown.ini` used for child exit. */
    [[nodiscard]] std::chrono::milliseconds shutdown_timeout();

    /**
     * \brief Registers hosted hello catalogs and enabled one-shot commands.
     *
     * Main-local names must already be registered so they win collisions.
     * On-demand commands come from `AC_LAUNCH_DESCRIPTOR` in the executable.
     */
    void register_with(command_registry::Registry& registry);

    /**
     * \brief Forwards `expression` to a started child on its control pipe.
     *
     * Logs and returns if that child is unavailable. Does not restart it.
     */
    void invoke(std::string_view component_name, std::string_view expression);

    /**
     * Move-only session. A moved-from object is inactive. Copy is disabled.
     */
    class Session final {
    public:
        Session(const Session&) = delete;
        Session& operator=(const Session&) = delete;
        Session(Session&& other) noexcept;
        Session& operator=(Session&&) = delete;
        ~Session();

    private:
        friend Session initialize();

        Session() = default;

        bool active_ {true};
    };
}
