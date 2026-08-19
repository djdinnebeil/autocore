/**
 * \file clipboard.ixx
 * \brief Provides Unicode text operations for the Windows clipboard.
 *
 * Auto Core uses the clipboard and the Ctrl+V shortcut to insert text
 * into the active input field.
 *
 * \par Preservation scope
 * Clipboard snapshots preserve CF_UNICODETEXT only. The text-representation
 * API additionally converts legacy text and Explorer file lists into Unicode
 * text. HTML, rich text, images, and application-specific formats are not
 * recreated. See core/TODO.md for planned multi-format preservation work.
 */
module;

#include "ac_api.hpp"

export module auto_core.clipboard;

import std;

export namespace ac::clipboard {

    /** \brief Classifies a captured clipboard text representation. */
    enum class ClipboardTextKind {
        unicode_text,
        file_paths,
        empty,
        unsupported
    };

    /**
     * \brief A normalized text view of the current clipboard contents.
     * `text` carries Unicode text or newline-separated file paths and is empty
     * for the `empty` and `unsupported` kinds.
     */
    struct ClipboardTextRepresentation {
        ClipboardTextKind kind;
        std::wstring text;
    };

    /** \brief Errors produced by clipboard and paste operations. */
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

    /** \brief Returns a stable user-facing description of an error. */
    [[nodiscard]]
    AC_API std::string_view error_message(Error error) noexcept;

    /**
     * \brief Replaces the clipboard with Unicode text.
     * \param text The text to store as `CF_UNICODETEXT`.
     * \return Success or the clipboard operation that failed.
     */
    [[nodiscard]]
    AC_API std::expected<void, Error>
        set_clipboard_text(std::wstring_view text);

    /**
     * \brief Reads Unicode text from the clipboard.
     * \return The text, or `unicode_text_unavailable` when Unicode text is
     * not present.
     */
    [[nodiscard]]
    AC_API std::expected<std::wstring, Error>
        get_clipboard_text();

    /**
     * \brief Captures a useful Unicode-text representation of the clipboard.
     *
     * Unicode and legacy text become Unicode text. Explorer file lists become
     * newline-separated absolute paths. Empty and unsupported clipboard data
     * remain distinguishable so callers can select an explicit fallback.
     */
    [[nodiscard]]
    AC_API std::expected<ClipboardTextRepresentation, Error>
        capture_clipboard_text_representation();

    /**
     * \brief A snapshot of the clipboard's Unicode text representation.
     *
     * A value contains the captured CF_UNICODETEXT data. std::nullopt means
     * that Unicode text was unavailable; it does not mean that the clipboard
     * contained no data in another format.
     */
    using ClipboardTextSnapshot = std::optional<std::wstring>;

    /**
     * \brief Captures the clipboard's Unicode-text state for restoration.
     * \return Captured text, an empty optional when Unicode text is absent,
     * or a clipboard access error.
     */
    [[nodiscard]]
    AC_API std::expected<ClipboardTextSnapshot, Error>
        capture_clipboard_text();

    /**
     * \brief Restores the Unicode-text state represented by a snapshot.
     *
     * A snapshot containing text replaces the clipboard with CF_UNICODETEXT.
     * An empty snapshot empties the clipboard. Formats not represented by
     * ClipboardTextSnapshot are not restored.
     *
     * \param snapshot The Unicode-text state to restore.
     */
    [[nodiscard]]
    AC_API std::expected<void, Error>
        restore_clipboard_text(const ClipboardTextSnapshot& snapshot);

    /**
     * \brief Sends Ctrl+V to the active input target.
     *
     * Input processing is asynchronous. Callers that replace the clipboard
     * immediately afterward must allow the target time to process the paste.
     */
    [[nodiscard]]
    AC_API std::expected<void, Error>
        paste_from_clipboard();
} // namespace ac::clipboard
