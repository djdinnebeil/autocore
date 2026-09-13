/**
 * \file ac_components.ixx
 * \brief Aggregates the Main-side interfaces for Auto Core components.
 */
export module auto_core.main.components;

export import auto_core.main.components.dash;
export import auto_core.main.components.itunes;
export import auto_core.main.components.journal;
export import auto_core.main.components.server;
export import auto_core.main.components.slash;
export import auto_core.main.components.spotify;
export import auto_core.main.components.taskbar;
export import auto_core.main.components.wake;
export import auto_core.main.components.writer;

export namespace ac::main::components {
    class Session;

    /**
     * \brief Starts taskbar, pipes, and child processes.
     * \return An RAII session whose destructor stops the taskbar client only.
     *         Pipe shutdowns run from `close_program()`.
     */
    [[nodiscard]] Session initialize();

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
