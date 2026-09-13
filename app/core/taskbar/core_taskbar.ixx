/**
 * \file core_taskbar.ixx
 * \brief Shared native taskbar-position support for Auto Core processes.
 */
module;

#include "../include/ac_api.hpp"

export module auto_core.taskbar;

import std;

export namespace ac::taskbar {

    /** A logical taskbar position. Position 10 is translated to key 0 only
        when keyboard input is generated. */
    struct Position {
        std::uint8_t value {};

        [[nodiscard]] constexpr bool valid() const noexcept {
            return value >= 1 && value <= 10;
        }

        friend constexpr bool operator==(Position, Position) = default;
    };

    /** How the current process-local snapshot was produced. */
    enum class SnapshotSource : std::uint8_t {
        /** Built from `taskbar/cached_positions.ini` without a live UI
            Automation pass. */
        configured,
        /** Built from a completed UI Automation discovery. */
        live,
        /** Taskbar mapping is disabled; lookups are empty. */
        disabled
    };

    /** Diagnostic metadata for the process-local immutable snapshot. */
    struct SnapshotInfo {
        /** Snapshot wire/schema version published by the authority. */
        std::uint32_t schema_version {};
        /** Monotonic generation; later values replace earlier ones. */
        std::uint64_t generation {};
        SnapshotSource source {SnapshotSource::configured};
        /** Number of first-ten native slots in the snapshot. */
        std::size_t slot_count {};
        /** Number of configured applications in the snapshot. */
        std::size_t application_count {};
        /** Number of friendly `activate_*` commands in the snapshot. */
        std::size_t configured_command_count {};
    };

    /** Diagnostic metadata for one of the first ten taskbar positions.
        `applications` contains the configured application keys routed to the
        position by the current immutable snapshot. */
    struct TaskbarSlotInfo {
        Position position;
        std::string automation_id;
        std::string application_id;
        std::string display_name;
        std::vector<std::string> applications;
    };

    /** On-demand metadata for one application button anywhere on the primary
        taskbar. `native_position` is present only for positions 1 through 10.
        Discovery performs UI Automation and is reserved for taskbar_ac.exe. */
    struct DiscoveredTaskbarApplication {
        std::size_t ordinal {};
        std::optional<Position> native_position;
        std::string automation_id;
        std::string application_id;
        std::string display_name;
    };

    /** A friendly public command supplied by application configuration. */
    struct ConfiguredActivationCommand {
        std::string command;
        std::string application;
    };

    /** A process-local decision produced from an immutable application route.
        A matching window count of 2 means two or more windows. Returned only
        when the application has a native Win+position mapping. */
    struct NativeActivationDecision {
        Position position;
        std::uint8_t matching_window_count {};
        bool should_cycle {};
    };

    /** An opaque Win32 HWND for taskbar window matching. */
    using WindowHandle = void*;

    /** Starts taskbar_ac.exe's session authority and the asynchronous initial
        calculation. Calling this more than once in the authority process is
        harmless. */
    [[nodiscard]] AC_API bool start_authority();

    /**
     * \brief Waits for a complete initial snapshot to be published.
     * \return `false` if the timeout elapses first. Does not start the
     * authority.
     */
    [[nodiscard]] AC_API bool wait_for_initial_snapshot(
        std::chrono::milliseconds timeout
    );

    /** Requests one asynchronous authority-owned recalculation. Concurrent
        requests are coalesced. This is called by taskbar_ac.exe. */
    [[nodiscard]] AC_API bool request_refresh();

    /** Stops taskbar_ac.exe's authority and all snapshot publisher threads. */
    AC_API void stop_authority() noexcept;

    /**
     * \brief Connects a client process to `taskbar_ac.exe` and receives the
     * complete snapshot.
     *
     * The default timeout is five seconds. Later generations are installed
     * by a background listener until `disconnect()`.
     */
    [[nodiscard]] AC_API bool connect(
        std::chrono::milliseconds timeout = std::chrono::seconds {5}
    );

    /**
     * \brief Stops this process's snapshot listener.
     *
     * Safe to call when not connected.
     */
    AC_API void disconnect() noexcept;

    /** Returns the current process-local position using only an in-memory
        lookup. */
    [[nodiscard]] AC_API std::optional<Position>
        get_native_taskbar_position(std::string_view application);

    /** Returns friendly activation commands from the current process-local
        immutable snapshot. This performs no file I/O or IPC. */
    [[nodiscard]] AC_API std::vector<ConfiguredActivationCommand>
        configured_activation_commands();

    /** Returns the optional executable launched when an application has no
        matching windows and no native Win+position mapping. This performs
        no file I/O or IPC. */
    [[nodiscard]] AC_API std::optional<std::wstring>
        configured_fallback_executable_path(std::string_view application);

    /** Returns whether the application has a process-local snapshot route,
        including applications with no native Win+position mapping. */
    [[nodiscard]] AC_API bool application_is_configured(
        std::string_view application
    );

    /** Returns whether the application's configuration requests interactive
        cycling when two or more windows match. */
    [[nodiscard]] AC_API bool configured_multi_window_cycle(
        std::string_view application
    );

    /** Enumerates visible titled top-level windows matching the application's
        configured [window] identity, in z-order. This may enumerate windows,
        but performs no UI Automation, file I/O, or IPC. */
    [[nodiscard]] AC_API std::vector<WindowHandle>
        matching_windows(std::string_view application);

    /** Evaluates configured multi-window behavior using the current local
        snapshot. Returns a decision only when the application has a native
        Win+position mapping. This may enumerate top-level windows, but
        performs no UI Automation, file I/O, or IPC. */
    [[nodiscard]] AC_API std::optional<NativeActivationDecision>
        prepare_native_activation(std::string_view application);

    /** Performs only native Win+position activation when the application is
        in the current process-local snapshot. This operation never launches
        an application and never performs UI Automation, file I/O, or IPC. */
    [[nodiscard]] AC_API bool try_activate_native(
        std::string_view application
    );

    /** Sends a one-shot shortcut for an already resolved logical position. */
    [[nodiscard]] AC_API bool activate_native_position(Position position);

    /** Holds Win and sends the first logical position for a Main-owned
        interactive cycling session. No cycling state is retained by DLL. */
    [[nodiscard]] AC_API bool begin_native_cycle(Position position);

    /** Sends another logical position while Main's cycling session holds
        Win. No cycling state is retained by the DLL. */
    [[nodiscard]] AC_API bool advance_native_cycle(Position position);

    /** Releases Win for Main's current cycling session. */
    AC_API void end_native_cycle() noexcept;

    /** Returns metadata for the process-local immutable snapshot. */
    [[nodiscard]] AC_API SnapshotInfo snapshot_info() noexcept;

    /** Returns a diagnostic copy of the first-ten taskbar metadata from the
        current process-local snapshot. This is intended for startup and
        inspection output, not the activation hot path. */
    [[nodiscard]] AC_API std::vector<TaskbarSlotInfo>
        first_ten_taskbar_slots();

    /** Performs an on-demand traversal of all primary-taskbar application
        buttons. This authority-only operation is used by taskbar_ac.exe for the
        standalone configuration wizard and is never part of activation. */
    [[nodiscard]] AC_API
        std::expected<std::vector<DiscoveredTaskbarApplication>, std::string>
        discover_taskbar_applications();
}
