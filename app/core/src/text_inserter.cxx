module auto_core.component;

import :text_inserter;
import std;
import auto_core.clipboard;

namespace ac::component_detail {

    namespace {
        // Serialize insertion transactions within this process.
        std::mutex insertion_mutex;

        // SendInput only queues Ctrl+V. Keep the temporary clipboard contents
        // available while the foreground application processes that input.
        constexpr auto paste_processing_delay =
            std::chrono::milliseconds {250};
    }

    std::string_view error_message(const InsertError error) noexcept {
        switch (error) {
        case InsertError::clipboard_write_failed:
            return "Unable to set clipboard text";
        case InsertError::paste_input_failed:
            return
                "Unable to paste from clipboard because "
                "the Ctrl+V input could not be sent";
        case InsertError::clipboard_restore_failed:
            return "Unable to restore the previous clipboard text";
        }

        return "Unable to insert text";
    }

    std::expected<void, InsertError>
        TextInserter::insert_replacing_clipboard(
            const std::wstring_view message
        ) {
        std::scoped_lock lock(insertion_mutex);

        if (!ac::clipboard::set_clipboard_text(message)) {
            return std::unexpected(InsertError::clipboard_write_failed);
        }

        if (!ac::clipboard::paste_from_clipboard()) {
            return std::unexpected(InsertError::paste_input_failed);
        }

        return {};
    }

    std::expected<void, InsertError>
        TextInserter::insert_preserving_clipboard_text(
            const std::wstring_view message,
            const ac::clipboard::ClipboardTextSnapshot& previous_clipboard
        ) {
        std::scoped_lock lock(insertion_mutex);

        if (!ac::clipboard::set_clipboard_text(message)) {
            (void)ac::clipboard::restore_clipboard_text(
                previous_clipboard
            );
            return std::unexpected(InsertError::clipboard_write_failed);
        }

        if (!ac::clipboard::paste_from_clipboard()) {
            (void)ac::clipboard::restore_clipboard_text(
                previous_clipboard
            );
            return std::unexpected(InsertError::paste_input_failed);
        }

        std::this_thread::sleep_for(paste_processing_delay);

        if (!ac::clipboard::restore_clipboard_text(
            previous_clipboard
        )) {
            return std::unexpected(
                InsertError::clipboard_restore_failed
            );
        }

        return {};
    }

}
