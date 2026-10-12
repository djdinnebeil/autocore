/**
 * \file initialization_sequence.ixx
 * \brief Ordered first-run owners launched by auto_core_init.exe.
 *
 * Main configuration owners run before the optional components_init.exe
 * stage. The Auto Core menu does not select that stage. Editors run after
 * it, so the component list is seeded from INI files that component
 * initialization may already have created. No init executable writes an
 * INI file.
 * logging.ini is established before this sequence.
 */
export module auto_core_initialization;

import std;

export namespace ac::main::init {

    enum class OwnerKind {
        configured,
        components,
        components_editor,
        keymap_editor
    };

    /**
     * \brief How Main configuration owners are launched.
     *
     * This does not select `components_init.exe`. `seed` is `--seed`.
     * `interactive` is `--init` and never includes `--seed`.
     * `seeded_interactive` passes both flags.
     */
    enum class InitializationMode {
        seed,
        interactive,
        seeded_interactive
    };

    /**
     * \brief Whether to launch components_init.exe, and with which flag.
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
    };

    inline constexpr std::array<OwnerStep, 8> initialization_steps {{
        {"auto_core_config.exe", OwnerKind::configured},
        {"components_config.exe", OwnerKind::configured},
        {"shutdown_config.exe", OwnerKind::configured},
        {"crash_recovery_config.exe", OwnerKind::configured},
        {"keymap_config.exe", OwnerKind::configured},
        {"components_init.exe", OwnerKind::components},
        {"components_editor.exe", OwnerKind::components_editor},
        {"keymap_editor.exe", OwnerKind::keymap_editor}
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
        case OwnerKind::components:
            return L"";
        case OwnerKind::components_editor:
            return L"--seed";
        case OwnerKind::keymap_editor:
            return L"";
        }
        return L"";
    }

    [[nodiscard]]
    constexpr std::wstring_view component_arguments(
        const ComponentLaunch launch
    ) noexcept {
        switch (launch) {
        case ComponentLaunch::seed:
            return L"--seed";
        case ComponentLaunch::configure:
            return L"--init";
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
