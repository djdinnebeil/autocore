module clipboard;

import std;
import keyboard;
import <Windows.h>;

namespace {

    class ClipboardSession {
    public:
        ClipboardSession()
            : opened_(OpenClipboard(nullptr) != FALSE) {
        }

        ~ClipboardSession() {
            if (opened_) {
                CloseClipboard();
            }
        }

        ClipboardSession(const ClipboardSession&) = delete;
        ClipboardSession& operator=(const ClipboardSession&) = delete;

        ClipboardSession(ClipboardSession&&) = delete;
        ClipboardSession& operator=(ClipboardSession&&) = delete;

        [[nodiscard]]
        bool is_open() const noexcept {
            return opened_;
        }

    private:
        bool opened_;
    };


    class OwnedGlobalMemory {
    public:
        explicit OwnedGlobalMemory(std::size_t size)
            : handle_(GlobalAlloc(GMEM_MOVEABLE, size)) {
        }

        ~OwnedGlobalMemory() {
            if (handle_ != nullptr) {
                GlobalFree(handle_);
            }
        }

        OwnedGlobalMemory(const OwnedGlobalMemory&) = delete;
        OwnedGlobalMemory& operator=(const OwnedGlobalMemory&) = delete;

        OwnedGlobalMemory(OwnedGlobalMemory&&) = delete;
        OwnedGlobalMemory& operator=(OwnedGlobalMemory&&) = delete;

        [[nodiscard]]
        HGLOBAL get() const noexcept {
            return handle_;
        }

        [[nodiscard]]
        explicit operator bool() const noexcept {
            return handle_ != nullptr;
        }

        HGLOBAL release() noexcept {
            HGLOBAL handle = handle_;
            handle_ = nullptr;
            return handle;
        }

    private:
        HGLOBAL handle_;
    };


    class GlobalLockGuard {
    public:
        explicit GlobalLockGuard(HGLOBAL handle)
            : handle_(handle),
            data_(GlobalLock(handle)) {
        }

        ~GlobalLockGuard() {
            if (data_ != nullptr) {
                GlobalUnlock(handle_);
            }
        }

        GlobalLockGuard(const GlobalLockGuard&) = delete;
        GlobalLockGuard& operator=(const GlobalLockGuard&) = delete;

        GlobalLockGuard(GlobalLockGuard&&) = delete;
        GlobalLockGuard& operator=(GlobalLockGuard&&) = delete;

        [[nodiscard]]
        void* get() const noexcept {
            return data_;
        }

        [[nodiscard]]
        explicit operator bool() const noexcept {
            return data_ != nullptr;
        }

    private:
        HGLOBAL handle_;
        void* data_;
    };

}


namespace ac::clipboard {

    std::string_view error_message(Error error) noexcept {
        switch (error) {
        case Error::open_failed:
            return "Failed to open clipboard";

        case Error::empty_failed:
            return "Failed to empty clipboard";

        case Error::text_too_large:
            return "Clipboard text is too large";

        case Error::allocation_failed:
            return "Failed to allocate clipboard memory";

        case Error::lock_failed:
            return "Failed to lock clipboard memory";

        case Error::unicode_text_unavailable:
            return "No Unicode text found in clipboard";

        case Error::get_data_failed:
            return "Failed to retrieve clipboard data";

        case Error::set_data_failed:
            return "Failed to set clipboard data";

        case Error::paste_failed:
            return "Failed to send Ctrl+V input";
        }

        return "Unknown clipboard error";
    }


    /**
     * \brief Sets the Unicode text content of the clipboard.
     * \param text The text to place on the clipboard.
     * \return Success or a clipboard error.
     */
    std::expected<void, Error>
        set_clipboard_text(std::wstring_view text) {

        ClipboardSession clipboard;

        if (!clipboard.is_open()) {
            return std::unexpected(Error::open_failed);
        }

        if (!EmptyClipboard()) {
            return std::unexpected(Error::empty_failed);
        }

        constexpr std::size_t wchar_size = sizeof(wchar_t);

        if (text.size() >
            ((std::numeric_limits<std::size_t>::max)() / wchar_size) - 1) {
            return std::unexpected(Error::text_too_large);
        }

        const std::size_t buffer_size =
            (text.size() + 1) * wchar_size;

        OwnedGlobalMemory clipboard_memory {buffer_size};

        if (!clipboard_memory) {
            return std::unexpected(Error::allocation_failed);
        }

        GlobalLockGuard locked_memory {clipboard_memory.get()};

        if (!locked_memory) {
            return std::unexpected(Error::lock_failed);
        }

        std::memcpy(
            locked_memory.get(),
            text.data(),
            text.size() * wchar_size
        );

        auto* destination =
            static_cast<wchar_t*>(locked_memory.get());

        destination[text.size()] = L'\0';

        if (SetClipboardData(
            CF_UNICODETEXT,
            clipboard_memory.get()
        ) == nullptr) {
            return std::unexpected(Error::set_data_failed);
        }

        clipboard_memory.release();

        return {};
    }


    /**
     * \brief Retrieves Unicode text from the clipboard.
     * \return The clipboard text or a clipboard error.
     */
    std::expected<std::wstring, Error>
        get_clipboard_text() {

        ClipboardSession clipboard;

        if (!clipboard.is_open()) {
            return std::unexpected(Error::open_failed);
        }

        if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            return std::unexpected(Error::unicode_text_unavailable);
        }

        HGLOBAL clipboard_memory =
            static_cast<HGLOBAL>(
                GetClipboardData(CF_UNICODETEXT)
                );

        if (clipboard_memory == nullptr) {
            return std::unexpected(Error::get_data_failed);
        }

        GlobalLockGuard locked_memory {clipboard_memory};

        if (!locked_memory) {
            return std::unexpected(Error::lock_failed);
        }

        const auto* text_data =
            static_cast<const wchar_t*>(locked_memory.get());

        return std::wstring {text_data};
    }


    /**
     * \brief Captures Unicode clipboard text for later restoration.
     * \return The captured text, no value when Unicode text is unavailable,
     * or a clipboard error.
     */
    std::expected<ClipboardTextSnapshot, Error>
        capture_clipboard_text() {

        auto result = get_clipboard_text();

        if (result) {
            return ClipboardTextSnapshot {std::move(*result)};
        }

        if (result.error() == Error::unicode_text_unavailable) {
            return ClipboardTextSnapshot {};
        }

        return std::unexpected(result.error());
    }


    /**
     * \brief Restores a previously captured Unicode clipboard snapshot.
     * \return Success when the snapshot has no Unicode text or was restored,
     * or a clipboard error.
     */
    std::expected<void, Error>
        restore_clipboard_text(const ClipboardTextSnapshot& snapshot) {

        if (!snapshot) {
            return {};
        }

        return set_clipboard_text(*snapshot);
    }


    /**
     * \brief Simulates Ctrl+V to paste the current clipboard contents.
     *
     * The generated input is processed asynchronously by the target
     * application. Callers that replace the clipboard immediately afterward
     * must allow the target application time to process the paste command.
     *
     * \return Success or a clipboard error.
     */
    std::expected<void, Error>
        paste_from_clipboard() {

        constexpr std::array keys {
            static_cast<std::uint8_t>(VK_CONTROL),
            static_cast<std::uint8_t>('V')
        };

        if (!ac::keyboard::send_key_combination(keys)) {
            return std::unexpected(Error::paste_failed);
        }

        return {};
    }

}
