/**
 * \file initialization_sequence.ixx
 * \brief Ordered first-run owners launched by auto_core_settings.exe --initialize.
 *
 * Main configuration owners run before the optional component
 * initialization stage of auto_core_components.exe. Taskbar and Writer
 * are the last owners in that group. The Auto Core menu
 * does not select that stage. The list edit runs after it, so
 * components.list is seeded from INI files that component initialization
 * may already have created. That initialization stage does not write an
 * INI file.
 * A missing logging_policy.ini is seeded before this sequence. The Main
 * menu decides whether that new policy stays enabled.
 */
export module auto_core_initialization;

import std;

export namespace ac::main::init {

    enum class OwnerKind {
        configured,
        taskbar,
        writer,
        components,
        components_editor,
        keymap
    };

    /**
     * \brief How Main configuration owners are launched.
     *
     * This does not select component initialization. `seed` is `--seed`.
     * `interactive` is `--init` and never includes `--seed`.
     * `seeded_interactive` passes both flags.
     */
    enum class InitializationMode {
        seed,
        interactive,
        seeded_interactive
    };

    /**
     * \brief Whether to launch component initialization, and with which flags.
     *
     * Chosen by the Components menu, or forced to `seed` for plain `--seed`.
     */
    enum class ComponentLaunch {
        seed,
        configure,
        skip
    };

    struct OwnerStep {
        std::string_view executable;
        OwnerKind kind;
        std::wstring_view route;
    };

    inline constexpr std::array<OwnerStep, 11> initialization_steps {{
        {"auto_core_settings.exe", OwnerKind::configured, L"--config --area auto-core"},
        {"auto_core_settings.exe", OwnerKind::configured, L"--config --area components-policy"},
        {"auto_core_settings.exe", OwnerKind::configured, L"--config --area shutdown"},
        {"auto_core_settings.exe", OwnerKind::configured, L"--config --area crash-recovery"},
        {"auto_core_settings.exe", OwnerKind::configured, L"--config --area startup"},
        {"auto_core_settings.exe", OwnerKind::configured, L"--config --area warnings"},
        {"auto_core_taskbar.exe", OwnerKind::taskbar, L"--config"},
        {"auto_core_writer.exe", OwnerKind::writer, L"--config"},
        {"auto_core_components.exe", OwnerKind::components, L""},
        {"auto_core_components.exe", OwnerKind::components_editor, L""},
        {"auto_core_keymap.exe", OwnerKind::keymap, L""}
    }};

    [[nodiscard]]
    constexpr std::wstring_view arguments_for(
        const OwnerKind kind,
        const InitializationMode mode
    ) noexcept {
        switch (kind) {
        case OwnerKind::configured:
            switch (mode) {
            case InitializationMode::seed:
                return L"--seed";
            case InitializationMode::interactive:
                return L"--init";
            case InitializationMode::seeded_interactive:
                return L"--init --seed";
            }
            return L"";
        case OwnerKind::taskbar:
        case OwnerKind::writer:
            switch (mode) {
            case InitializationMode::seed:
                return L"--seed";
            case InitializationMode::interactive:
                return L"--init";
            case InitializationMode::seeded_interactive:
                return L"--init --seed";
            }
            return L"";
        case OwnerKind::components:
            return L"";
        case OwnerKind::components_editor:
            return L"--edit --seed";
        case OwnerKind::keymap:
            return L"--seed";
        }
        return L"";
    }

    [[nodiscard]]
    constexpr std::wstring_view component_arguments(
        const ComponentLaunch launch
    ) noexcept {
        switch (launch) {
        case ComponentLaunch::seed:
            return L"--initialize --seed";
        case ComponentLaunch::configure:
            return L"--initialize";
        case ComponentLaunch::skip:
            return L"";
        }
        return L"";
    }

    /** Every stage must succeed. A failure removes a newly created auto_core.ini. */
    [[nodiscard]]
    constexpr bool stops_on_failure(const OwnerKind) noexcept {
        return true;
    }

} // namespace ac::main::init
