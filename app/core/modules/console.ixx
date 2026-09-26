/**
 * \file console.ixx
 * \brief Provides access to the console shared by Auto Core components.
 *
 * Generic `{name}_ac.exe` children inherit Auto Core's console so `print()`
 * shares `std::cout`. Interactive prompts clear pending input and activate
 * that console.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.console;

import std;

export namespace ac::console {

    /** \brief An opaque Win32 window handle. */
    using WindowHandle = void*;

    /** \brief Errors produced while locating or activating windows. */
    enum class Error {
        console_unavailable,
        activation_failed,
        /** The HWND passed to `activate_window()` or `restore_focus()` is
            null or no longer a window. Not limited to a previous foreground
            window. */
        window_unavailable,
        window_activation_failed,
        /** Reserved. `focus_for_prompt_via_winkey()` never returns this;
            Win+number failure falls back to `activate()`. */
        winkey_activation_failed
    };

    /** \brief Returns a stable user-facing description of an error. */
    [[nodiscard]]
    AC_API std::string_view error_message(Error error) noexcept;

    /**
     * \brief Returns the window for this process's attached console.
     * \return The console window, or null when no console is attached.
     */
    [[nodiscard]]
    AC_API WindowHandle window() noexcept;

    /**
     * \brief Activates and verifies the Auto Core console.
     *
     * Attaches to the parent console when this process has none. Uses only
     * direct Windows activation mechanisms. Application-specific fallbacks,
     * such as taskbar shortcuts, belong to the caller.
     *
     * \return Success, `console_unavailable`, or `activation_failed`.
     */
    [[nodiscard]]
    AC_API std::expected<void, Error> activate() noexcept;

    /**
     * \brief Restores, activates, and verifies an arbitrary top-level window.
     * \param target_window The window to activate.
     * \return Success, `window_unavailable`, or `window_activation_failed`.
     */
    [[nodiscard]]
    AC_API std::expected<void, Error>
        activate_window(WindowHandle target_window) noexcept;

    /**
     * \brief Clears pending input, activates the Auto Core console, and
     * returns the window that was foreground before the prompt began.
     *
     * Attaches to the parent console when this process has none.
     * \return The previously foreground window, or a console activation error.
     */
    [[nodiscard]]
    AC_API std::expected<WindowHandle, Error> focus_for_prompt();

    /**
     * \brief Focuses the Auto Core console through Win+number when available.
     *
     * Attaches to the parent console when this process has none, or allocates
     * a console if that attach fails. Uses the
     * process-local `auto_core` taskbar slot when a snapshot has one. No
     * snapshot, or a snapshot with no `auto_core` mapping, uses direct
     * activation. If Win+number cannot focus the console, `activate()` is
     * used; this function does not return `winkey_activation_failed`. The
     * current foreground window is returned for `restore_focus()`. This is
     * not a substitute for `taskbar_ac.exe` snapshot authority.
     */
    [[nodiscard]]
    AC_API std::expected<WindowHandle, Error>
        focus_for_prompt_via_winkey();

    /**
     * \brief Restores and verifies a previously captured foreground window.
     * \param previous_window A handle returned by `focus_for_prompt()` or
     * `focus_for_prompt_via_winkey()`.
     * \return The result of activating that window.
     */
    [[nodiscard]]
    AC_API std::expected<void, Error>
        restore_focus(WindowHandle previous_window) noexcept;
} // namespace ac::console
