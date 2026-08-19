/**
 * \file console.ixx
 * \brief Provides access to the console shared by Auto Core components.
 *
 * Components launched normally by Auto Core inherit its console. Interactive
 * prompts clear pending input before activating that attached console.
 */
module;

#include "ac_api.hpp"

export module auto_core.console;

import std;

export namespace ac::console {

    /** \brief An opaque Win32 window handle. */
    using WindowHandle = void*;

    /** \brief Errors produced while locating or activating windows. */
    enum class Error {
        console_unavailable,
        activation_failed,
        window_unavailable,
        window_activation_failed
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
     * \brief Activates and verifies the console attached to this process.
     *
     * Uses only direct Windows activation mechanisms. Application-specific
     * fallbacks, such as taskbar shortcuts, belong to the caller.
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
     * \brief Clears pending input, activates the attached console, and returns
     * the window that was foreground before the prompt began.
     * \return The previously foreground window, or a console activation error.
     */
    [[nodiscard]]
    AC_API std::expected<WindowHandle, Error> focus_for_prompt();

    /**
     * \brief Restores and verifies a previously captured foreground window.
     * \param previous_window A handle returned by `focus_for_prompt()`.
     * \return The result of activating that window.
     */
    [[nodiscard]]
    AC_API std::expected<void, Error>
        restore_focus(WindowHandle previous_window) noexcept;
} // namespace ac::console
