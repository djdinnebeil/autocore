module auto_core.clipboard;

import std;
import auto_core.keyboard;
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

    std::expected<std::wstring, ac::clipboard::Error>
        read_legacy_clipboard_text(
            const UINT format,
            const UINT code_page
        ) {
        const HGLOBAL memory = static_cast<HGLOBAL>(
            GetClipboardData(format)
        );

        if (memory == nullptr) {
            return std::unexpected(
                ac::clipboard::Error::get_data_failed
            );
        }

        const SIZE_T memory_size = GlobalSize(memory);
        GlobalLockGuard locked_memory {memory};

        if (!locked_memory) {
            return std::unexpected(ac::clipboard::Error::lock_failed);
        }

        const auto* begin = static_cast<const char*>(locked_memory.get());
        const auto* end = begin + memory_size;
        const auto* terminator = std::find(begin, end, '\0');

        if (terminator == end) {
            return std::unexpected(
                ac::clipboard::Error::get_data_failed
            );
        }

        const auto legacy_size = terminator - begin;

        if (legacy_size > (std::numeric_limits<int>::max)()) {
            return std::unexpected(
                ac::clipboard::Error::text_too_large
            );
        }

        const auto input_size = static_cast<int>(legacy_size);

        if (input_size == 0) {
            return std::wstring {};
        }

        const int output_size = MultiByteToWideChar(
            code_page,
            0,
            begin,
            input_size,
            nullptr,
            0
        );

        if (output_size == 0) {
            return std::unexpected(
                ac::clipboard::Error::get_data_failed
            );
        }

        std::wstring result(
            static_cast<std::size_t>(output_size),
            L'\0'
        );

        if (MultiByteToWideChar(
            code_page,
            0,
            begin,
            input_size,
            result.data(),
            output_size
        ) == 0) {
            return std::unexpected(
                ac::clipboard::Error::get_data_failed
            );
        }

        return result;
    }

    struct DropFilesHeader {
        DWORD file_offset;
        POINT drop_point;
        BOOL non_client_area;
        BOOL wide_characters;
    };

    std::expected<std::wstring, ac::clipboard::Error>
        read_file_drop_paths() {
        const HGLOBAL memory = static_cast<HGLOBAL>(
            GetClipboardData(CF_HDROP)
        );

        if (memory == nullptr) {
            return std::unexpected(
                ac::clipboard::Error::get_data_failed
            );
        }

        const SIZE_T memory_size = GlobalSize(memory);
        GlobalLockGuard locked_memory {memory};

        if (!locked_memory) {
            return std::unexpected(ac::clipboard::Error::lock_failed);
        }

        if (memory_size < sizeof(DropFilesHeader)) {
            return std::unexpected(
                ac::clipboard::Error::get_data_failed
            );
        }

        const auto* bytes =
            static_cast<const std::byte*>(locked_memory.get());
        const auto* header =
            reinterpret_cast<const DropFilesHeader*>(bytes);

        if (header->file_offset >= memory_size) {
            return std::unexpected(
                ac::clipboard::Error::get_data_failed
            );
        }

        std::wstring paths;

        if (header->wide_characters) {
            if (
                header->file_offset % alignof(wchar_t) != 0 ||
                (memory_size - header->file_offset) < sizeof(wchar_t)
            ) {
                return std::unexpected(
                    ac::clipboard::Error::get_data_failed
                );
            }

            const auto* current = reinterpret_cast<const wchar_t*>(
                bytes + header->file_offset
            );
            const auto* end = current +
                ((memory_size - header->file_offset) / sizeof(wchar_t));

            while (current < end && *current != L'\0') {
                const auto* terminator = std::find(current, end, L'\0');

                if (terminator == end) {
                    return std::unexpected(
                        ac::clipboard::Error::get_data_failed
                    );
                }

                if (!paths.empty()) {
                    paths += L'\n';
                }

                paths.append(current, terminator);
                current = terminator + 1;
            }
        }
        else {
            const auto* current = reinterpret_cast<const char*>(
                bytes + header->file_offset
            );
            const auto* end = reinterpret_cast<const char*>(
                bytes + memory_size
            );

            while (current < end && *current != '\0') {
                const auto* terminator = std::find(current, end, '\0');

                if (terminator == end) {
                    return std::unexpected(
                        ac::clipboard::Error::get_data_failed
                    );
                }

                const auto path_size = terminator - current;

                if (path_size > (std::numeric_limits<int>::max)()) {
                    return std::unexpected(
                        ac::clipboard::Error::text_too_large
                    );
                }

                const int input_size = static_cast<int>(path_size);
                const int output_size = MultiByteToWideChar(
                    CP_ACP,
                    0,
                    current,
                    input_size,
                    nullptr,
                    0
                );

                if (output_size == 0) {
                    return std::unexpected(
                        ac::clipboard::Error::get_data_failed
                    );
                }

                std::wstring path(
                    static_cast<std::size_t>(output_size),
                    L'\0'
                );

                if (MultiByteToWideChar(
                    CP_ACP,
                    0,
                    current,
                    input_size,
                    path.data(),
                    output_size
                ) == 0) {
                    return std::unexpected(
                        ac::clipboard::Error::get_data_failed
                    );
                }

                if (!paths.empty()) {
                    paths += L'\n';
                }

                paths += path;
                current = terminator + 1;
            }
        }

        return paths;
    }

}


namespace ac::clipboard {

    std::expected<ClipboardTextRepresentation, Error>
        capture_clipboard_text_representation() {
        auto unicode_text = get_clipboard_text();

        if (unicode_text) {
            return ClipboardTextRepresentation {
                .kind = ClipboardTextKind::unicode_text,
                .text = std::move(*unicode_text)
            };
        }

        if (unicode_text.error() != Error::unicode_text_unavailable) {
            return std::unexpected(unicode_text.error());
        }

        ClipboardSession clipboard;

        if (!clipboard.is_open()) {
            return std::unexpected(Error::open_failed);
        }

        if (CountClipboardFormats() == 0) {
            return ClipboardTextRepresentation {
                .kind = ClipboardTextKind::empty,
                .text = {}
            };
        }

        if (IsClipboardFormatAvailable(CF_TEXT)) {
            auto legacy_text =
                read_legacy_clipboard_text(CF_TEXT, CP_ACP);

            if (!legacy_text) {
                return std::unexpected(legacy_text.error());
            }

            return ClipboardTextRepresentation {
                .kind = ClipboardTextKind::unicode_text,
                .text = std::move(*legacy_text)
            };
        }

        if (IsClipboardFormatAvailable(CF_OEMTEXT)) {
            auto legacy_text =
                read_legacy_clipboard_text(CF_OEMTEXT, CP_OEMCP);

            if (!legacy_text) {
                return std::unexpected(legacy_text.error());
            }

            return ClipboardTextRepresentation {
                .kind = ClipboardTextKind::unicode_text,
                .text = std::move(*legacy_text)
            };
        }

        if (IsClipboardFormatAvailable(CF_HDROP)) {
            auto paths = read_file_drop_paths();

            if (!paths) {
                return std::unexpected(paths.error());
            }

            if (!paths->empty()) {
                return ClipboardTextRepresentation {
                    .kind = ClipboardTextKind::file_paths,
                    .text = std::move(*paths)
                };
            }
        }

        return ClipboardTextRepresentation {
            .kind = ClipboardTextKind::unsupported,
            .text = {}
        };
    }

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

        {
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
        }

        ClipboardSession clipboard;

        if (!clipboard.is_open()) {
            return std::unexpected(Error::open_failed);
        }

        if (!EmptyClipboard()) {
            return std::unexpected(Error::empty_failed);
        }

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
            ClipboardSession clipboard;

            if (!clipboard.is_open()) {
                return std::unexpected(Error::open_failed);
            }

            if (!EmptyClipboard()) {
                return std::unexpected(Error::empty_failed);
            }

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
