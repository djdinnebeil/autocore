/**
 * \file text_inserter.ixx
 * \brief Inserts Unicode text into the active input through the clipboard.
 */
export module auto_core.component:text_inserter;

import std;
import auto_core.clipboard;

namespace ac::component_detail {

    /** \brief Failures produced by a clipboard-based insertion transaction. */
    enum class InsertError {
        clipboard_write_failed,
        paste_input_failed,
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
         * \brief Replaces the clipboard with text and sends Ctrl+V.
         * \param message The Unicode text to insert.
         * \return Success or the failed clipboard/paste operation.
         */
        [[nodiscard]] std::expected<void, InsertError>
            insert_replacing_clipboard(std::wstring_view message);

        /**
         * \brief Inserts text and then restores a Unicode-text snapshot.
         *
         * Restoration is attempted after write or paste failures. After a
         * successful paste, the method waits briefly before restoration so
         * the target application can consume the temporary clipboard data.
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
