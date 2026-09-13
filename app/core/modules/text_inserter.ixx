/**
 * \file text_inserter.ixx
 * \brief Inserts Unicode text into the active input through the clipboard.
 */
export module auto_core.core.component:text_inserter;

import std;
import auto_core.core.clipboard;

namespace ac::component_detail {

    /** \brief Failures produced by a clipboard-based insertion transaction. */
    enum class InsertError {
        clipboard_write_failed,
        paste_input_failed,
        linebreak_input_failed,
        clipboard_restore_failed
    };

    /** \brief Returns a stable user-facing description of an insertion error. */
    [[nodiscard]] std::string_view
        error_message(InsertError error) noexcept;

    /**
     * \brief Serializes clipboard-based text insertion within this process.
     */
    class TextInserter {
    public:
        /**
         * \brief Replaces the clipboard with text, sends Ctrl+V, waits
         * briefly, and sends Shift+Enter once.
         *
         * The wait keeps Chromium-based editors from applying Shift+Enter
         * before they have processed the paste. The inserted text remains
         * on the clipboard.
         *
         * \param message The Unicode text to insert without a trailing
         * newline.
         * \return Success or the failed clipboard, paste, or linebreak input.
         */
        [[nodiscard]] std::expected<void, InsertError>
            insert_replacing_clipboard(std::wstring_view message);

        /**
         * \brief Inserts text and then restores a Unicode-text snapshot.
         *
         * Sequence: write the temporary text, Ctrl+V, wait briefly so the
         * target can consume that paste, Shift+Enter once, then restore.
         * Restoration is also attempted after write, paste, or linebreak
         * failures so the previous snapshot is not left displaced.
         *
         * \param message The temporary Unicode clipboard text.
         * \param previous_clipboard The Unicode-text state to restore.
         */
        [[nodiscard]] std::expected<void, InsertError>
            insert_preserving_clipboard_text(
                std::wstring_view message,
                const ac::clipboard::ClipboardTextSnapshot& previous_clipboard
            );

    };

} // namespace ac::component_detail
