/**
 * \file initialization_sequence.ixx
 * \brief Ordered first-run owners launched by auto_core_init.exe.
 *
 * The Auto Core choice selects only auto_core_config.exe. logger_init.exe
 * and components_init.exe make their own decisions. No init executable
 * writes an INI file.
 */
export module auto_core_initialization;

import std;

export namespace ac::main::init {

    enum class OwnerKind {
        auto_core,
        logger,
        components,
        seeded_config,
        components_editor,
        keymap_editor
    };

    struct OwnerStep {
        std::string_view executable;
        OwnerKind kind;
    };

    inline constexpr std::array<OwnerStep, 9> initialization_steps {{
        {"auto_core_config.exe", OwnerKind::auto_core},
        {"logger_init.exe", OwnerKind::logger},
        {"components_init.exe", OwnerKind::components},
        {"keymap_config.exe", OwnerKind::seeded_config},
        {"components_config.exe", OwnerKind::seeded_config},
        {"shutdown_config.exe", OwnerKind::seeded_config},
        {"crash_recovery_config.exe", OwnerKind::seeded_config},
        {"components_editor.exe", OwnerKind::components_editor},
        {"keymap_editor.exe", OwnerKind::keymap_editor}
    }};

    /**
     * \brief Arguments for one owner.
     *
     * `auto_core_defaults` applies only to `auto_core_config.exe`.
     */
    [[nodiscard]]
    constexpr std::wstring_view arguments_for(
        const OwnerKind kind,
        const bool auto_core_defaults
    ) noexcept {
        switch (kind) {
        case OwnerKind::auto_core:
            return auto_core_defaults ? L"--seed" : L"--init";
        case OwnerKind::seeded_config:
        case OwnerKind::components_editor:
            return L"--seed";
        case OwnerKind::logger:
        case OwnerKind::components:
        case OwnerKind::keymap_editor:
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
