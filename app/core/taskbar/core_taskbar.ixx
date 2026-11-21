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
        /** Built from `data/taskbar/winkey_map.cache` without a live UI
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

    /** A friendly public command supplied by application configuration. */
    struct ConfiguredActivationCommand {
        std::string command;
        std::string application;
    };

    /** One activation command and its optional keymap catalog description.
        The text comes from `activation_catalog_description.hpp`. */
    struct ConfiguredActivationCatalogEntry {
        std::string command;
        std::string description;
    };

    /** One disk-map activation command, the application key that runs it,
        and its catalog description. A repeated command keeps the first
        application key and description. */
    struct ConfiguredActivationBinding {
        std::string command;
        std::string application;
        std::string description;
    };

    /** A provider offering one `activate_<program>` command. `application`
        is the Taskbar key used to activate. It is empty for a component. */
    struct ActivationOffer {
        std::string provider;
        std::string command;
        std::string description;
        std::string application;
    };

    /** An explicit alias. It always runs `command` on `provider`. */
    struct ActivationAlias {
        std::string name;
        std::string provider;
        std::string command;
        std::string description;
        std::string application;
    };

    /** The shared command and the aliases of the providers that offered it.
        `resolved` is false when the saved provider cannot be used.
        `blank_selection` is true when the saved value is missing or blank.
        `explicit_blank` is true only when a saved entry's value is empty.
        `offer_count` is the number of providers in the current discovery.
        `unset_description` is catalog text for an explicit blank that still
        has two or more providers. One provider with no saved entry does not
        produce a binding. */
    struct ActivationBinding {
        std::string command;
        bool resolved {false};
        bool blank_selection {false};
        bool explicit_blank {false};
        std::size_t offer_count {0};
        std::string provider;
        std::string description;
        std::string unset_description;
        std::string application;
        std::vector<ActivationAlias> aliases;
    };

    /** Bindings and human-readable reports. Reports do not by themselves
        reject a keymap build. */
    struct ActivationResolution {
        std::vector<ActivationBinding> bindings;
        std::vector<std::string> reports;
    };

    /** Result of the keymap build's rewrite of `activate_commands.map`.
        `ok` is false when the existing file could not be read or the new
        text could not be published. The previous file is left in place. */
    struct ActivationMapUpdate {
        bool ok {false};
        ActivationResolution resolution;
        std::string error;
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

    /** Starts auto_core_taskbar.exe's session authority and the asynchronous
        initial calculation. Calling this more than once in the authority
        process is harmless. */
    /**
     * \brief Replaces `winkey_map.cache` from one live taskbar discovery.
     *
     * Uses `<installation>/data/taskbar`. Does not start the snapshot authority
     * or the control pipe. `mode = cache` does not select the existing cache.
     *
     * \return `false` when live discovery fails or the cache file cannot be
     * replaced. A previous cache file is left in place.
     */
    [[nodiscard]] AC_API bool refresh_winkey_cache();

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
        requests are coalesced. This is called by auto_core_taskbar.exe. */
    [[nodiscard]] AC_API bool request_refresh();

    /** Stops auto_core_taskbar.exe's authority and all snapshot publisher threads. */
    AC_API void stop_authority() noexcept;

    /**
     * \brief Connects a client process to `auto_core_taskbar.exe` and receives the
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

    /**
     * \brief Returns activation command names parsed from the current
     * application maps.
     *
     * Reads `data/taskbar/applications/*.map` through the same loader the
     * authority uses. Does not start the authority, refresh positions, or
     * write configuration. A missing applications directory returns an
     * empty list.
     */
    [[nodiscard]] AC_API std::vector<std::string>
        configured_activation_commands_from_disk();

    /**
     * \brief Returns activation commands and catalog descriptions from maps.
     *
     * Uses the same loader as `configured_activation_commands_from_disk`.
     * A repeated command name keeps the first description. The result is
     * sorted by command name.
     */
    [[nodiscard]] AC_API std::vector<ConfiguredActivationCatalogEntry>
        configured_activation_catalog_from_disk();

    /**
     * \brief Returns disk-map activation commands with the application key
     * that runs each command.
     *
     * Uses the same loader and first-command rule as
     * `configured_activation_catalog_from_disk`.
     */
    [[nodiscard]] AC_API std::vector<ConfiguredActivationBinding>
        configured_activation_bindings_from_disk();

    /**
     * \brief Resolves shared activation names from `map_path` and `offers`.
     *
     * Does not create or rewrite the map. A missing file has no saved
     * selections. `occupied_names` are commands that already exist; a
     * generated alias that matches one is omitted and reported.
     */
    [[nodiscard]] AC_API ActivationResolution resolve_activation_commands(
        const std::filesystem::path& map_path,
        const std::vector<ActivationOffer>& offers,
        const std::vector<std::string>& occupied_names
    );

    /**
     * \brief Rewrites `activate_commands.map` from a successful provider discovery.
     *
     * Refreshes each generated `# Choice:` or `# Choices:` comment and
     * preserves saved values, including a blank. `# Choice:` is used when
     * one provider offers the command. `# Choices:` is used otherwise.
     * A new collision defaults to `taskbar` when that provider offered the
     * command, and is left blank otherwise. Returns the same resolution
     * `resolve_activation_commands` would return for the updated text.
     */
    [[nodiscard]] AC_API ActivationMapUpdate update_activation_map(
        const std::filesystem::path& map_path,
        const std::vector<ActivationOffer>& offers,
        const std::vector<std::string>& occupied_names
    );

    /**
     * \brief Resolves ordinary shared names from `map_path` and `offers`.
     *
     * Does not create or rewrite the map. A missing file has no saved
     * selections. Commands that start with `activate_` are ignored.
     * `occupied_names` are commands that already exist; a generated alias
     * that matches one is omitted and reported.
     */
    [[nodiscard]] AC_API ActivationResolution resolve_shared_commands(
        const std::filesystem::path& map_path,
        const std::vector<ActivationOffer>& offers,
        const std::vector<std::string>& occupied_names
    );

    /**
     * \brief Rewrites `shared_commands.map` from a successful provider discovery.
     *
     * Refreshes each generated `# Choice:` or `# Choices:` comment and
     * preserves saved values, including a blank, while any discovered
     * provider still registers the command. A saved command that none of
     * them registers is removed with its generated choice comment. The
     * caller must pass a complete successful discovery; this function does
     * not decide whether a missing provider was disabled or failed to
     * export. A new collision is left blank. An empty `[shared_commands]`
     * section is kept. Returns the same resolution `resolve_shared_commands`
     * would return for the updated text.
     */
    [[nodiscard]] AC_API ActivationMapUpdate update_shared_command_map(
        const std::filesystem::path& map_path,
        const std::vector<ActivationOffer>& offers,
        const std::vector<std::string>& occupied_names
    );

    /**
     * \brief Drops aliases that collide across both resolutions.
     *
     * The namespace starts with `occupied_names` and every command on either
     * resolution. Activation aliases are claimed first. A later alias that
     * matches a claimed name is removed and reported. `--build` and startup
     * both use this order.
     */
    AC_API void reconcile_provider_aliases(
        ActivationResolution& activation,
        ActivationResolution& shared,
        const std::vector<std::string>& occupied_names
    );

    /** Returns the optional executable launched when an application has no
        matching windows and no native Win+position mapping. This performs
        no file I/O or IPC. */
    [[nodiscard]] AC_API std::optional<std::wstring>
        configured_fallback_executable_path(std::string_view application);

    /** Returns the configured `[window] executable_path`, including the
        `::runtime::` sentinel. Empty when the application is absent.
        This performs no file I/O or IPC. */
    [[nodiscard]] AC_API std::optional<std::wstring>
        configured_window_executable_path(std::string_view application);

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

    /** Releases Win for Main's current cycling session. Returns whether the
        generated key-up event was accepted. */
    [[nodiscard]] AC_API bool end_native_cycle() noexcept;

    /** Returns metadata for the process-local immutable snapshot. */
    [[nodiscard]] AC_API SnapshotInfo snapshot_info() noexcept;

    /** Returns a diagnostic copy of the first-ten taskbar metadata from the
        current process-local snapshot. This is intended for startup and
        inspection output, not the activation hot path. */
    [[nodiscard]] AC_API std::vector<TaskbarSlotInfo>
        first_ten_taskbar_slots();
}
