/**
 * \file clipboard.ixx
 * \brief Provides Unicode text operations for the Windows clipboard.
 *
 * Auto Core uses the clipboard and the Ctrl+V shortcut to insert text
 * into the active input field.
 */
module;

#include "ac_api.hpp"

export module clipboard;

import std;

export namespace ac::clipboard {

    enum class Error {
        open_failed,
        empty_failed,
        text_too_large,
        allocation_failed,
        lock_failed,
        unicode_text_unavailable,
        get_data_failed,
        set_data_failed,
        paste_failed
    };

    [[nodiscard]]
    AC_API std::string_view error_message(Error error) noexcept;

    [[nodiscard]]
    AC_API std::expected<void, Error>
        set_clipboard_text(std::wstring_view text);

    [[nodiscard]]
    AC_API std::expected<std::wstring, Error>
        get_clipboard_text();

    using ClipboardTextSnapshot = std::optional<std::wstring>;

    [[nodiscard]]
    AC_API std::expected<ClipboardTextSnapshot, Error>
        capture_clipboard_text();

    [[nodiscard]]
    AC_API std::expected<void, Error>
        restore_clipboard_text(const ClipboardTextSnapshot& snapshot);

    [[nodiscard]]
    AC_API std::expected<void, Error>
        paste_from_clipboard();
}
