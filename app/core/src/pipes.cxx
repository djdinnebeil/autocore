module auto_core.pipes;

import std;

import <Windows.h>;

namespace {

    constexpr std::uint32_t maximum_message_size = 1024 * 1024;
    constexpr DWORD pipe_connect_timeout_ms = 5000;
    constexpr DWORD pipe_connect_retry_ms = 50;

    ac::pipes::Result<void> write_pipe_data(
        HANDLE h_pipe,
        const void* data,
        std::size_t size
    ) {
        const auto* cursor = static_cast<const std::byte*>(data);

        while (size != 0) {
            const DWORD requested = static_cast<DWORD>((std::min)(
                size,
                static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())
            ));
            DWORD bytes_written {};

            if (!WriteFile(h_pipe, cursor, requested, &bytes_written, nullptr)) {
                return std::unexpected(ac::pipes::Error { GetLastError() });
            }

            if (bytes_written == 0) {
                return std::unexpected(ac::pipes::Error { ERROR_BROKEN_PIPE });
            }

            cursor += bytes_written;
            size -= bytes_written;
        }

        return {};
    }

    ac::pipes::Result<void> read_pipe_data(
        HANDLE h_pipe,
        void* data,
        std::size_t size
    ) {
        auto* cursor = static_cast<std::byte*>(data);

        while (size != 0) {
            const DWORD requested = static_cast<DWORD>((std::min)(
                size,
                static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())
            ));
            DWORD bytes_read {};

            if (!ReadFile(h_pipe, cursor, requested, &bytes_read, nullptr)) {
                return std::unexpected(ac::pipes::Error { GetLastError() });
            }

            if (bytes_read == 0) {
                return std::unexpected(ac::pipes::Error { ERROR_BROKEN_PIPE });
            }

            cursor += bytes_read;
            size -= bytes_read;
        }

        return {};
    }

}

namespace ac::pipes {

    Pipe::Pipe() noexcept
        : handle_(INVALID_HANDLE_VALUE) {
    }

    Pipe::Pipe(const native_handle_type handle) noexcept
        : handle_(handle) {
    }

    Pipe::~Pipe() noexcept {
        reset();
    }

    Pipe::Pipe(Pipe&& other) noexcept
        : handle_(other.release()) {
    }

    Pipe& Pipe::operator=(Pipe&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    Pipe::native_handle_type Pipe::native_handle() const noexcept {
        return handle_;
    }

    bool Pipe::valid() const noexcept {
        return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
    }

    bool Pipe::cancel() noexcept {
        return valid() && CancelIoEx(handle_, nullptr) != FALSE;
    }

    Pipe::native_handle_type Pipe::release() noexcept {
        const native_handle_type handle = handle_;
        handle_ = INVALID_HANDLE_VALUE;
        return handle;
    }

    void Pipe::reset() noexcept {
        reset(INVALID_HANDLE_VALUE);
    }

    void Pipe::reset(const native_handle_type handle) noexcept {
        if (valid()) {
            CloseHandle(handle_);
        }
        handle_ = handle;
    }

    class CommandDispatcher::Impl {
    public:
        std::unordered_map<int, std::function<void()>> commands;
        std::atomic_bool stop_requested {false};
    };

    CommandDispatcher::CommandDispatcher()
        : impl_(std::make_unique<Impl>()) {
    }

    CommandDispatcher::~CommandDispatcher() = default;

    Result<Pipe> create_pipe_server(const std::wstring& pipe_name) {
        const std::wstring full_pipe_name = LR"(\\.\pipe\)" + pipe_name;

        HANDLE h_pipe = CreateNamedPipeW(
            full_pipe_name.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            4096,
            4096,
            0,
            nullptr
        );

        if (h_pipe == INVALID_HANDLE_VALUE) {
            return std::unexpected(Error { GetLastError() });
        }

        return Pipe {h_pipe};
    }

    Result<Pipe> connect_to_pipe_server(const std::wstring& pipe_name) {
        const std::wstring full_pipe_name = LR"(\\.\pipe\)" + pipe_name;
        const ULONGLONG deadline = GetTickCount64() + pipe_connect_timeout_ms;
        DWORD connect_error = ERROR_SUCCESS;

        while (true) {
            HANDLE h_pipe = CreateFileW(
                full_pipe_name.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr
            );

            if (h_pipe != INVALID_HANDLE_VALUE) {
                return Pipe {h_pipe};
            }

            connect_error = GetLastError();

            if (connect_error != ERROR_PIPE_BUSY &&
                connect_error != ERROR_FILE_NOT_FOUND) {
                break;
            }

            const ULONGLONG now = GetTickCount64();
            if (now >= deadline) {
                break;
            }

            const DWORD retry_wait = static_cast<DWORD>((std::min)(
                deadline - now,
                static_cast<ULONGLONG>(pipe_connect_retry_ms)
            ));

            if (connect_error == ERROR_PIPE_BUSY) {
                WaitNamedPipeW(full_pipe_name.c_str(), retry_wait);
            }
            else {
                Sleep(retry_wait);
            }
        }

        return std::unexpected(Error { connect_error });
    }

    Result<void> send_pipe_command(Pipe& pipe, int command) {
        return write_pipe_data(pipe.native_handle(), &command, sizeof(command));
    }

    void CommandDispatcher::set_command(
        int command,
        std::function<void()> action
    ) {
        impl_->commands.insert_or_assign(command, std::move(action));
    }

    void CommandDispatcher::request_stop() noexcept {
        impl_->stop_requested.store(true, std::memory_order_relaxed);
    }

    bool CommandDispatcher::stop_requested() const noexcept {
        return impl_->stop_requested.load(std::memory_order_relaxed);
    }

    Result<void> CommandDispatcher::process(Pipe& pipe) {
        while (!stop_requested()) {
            std::int32_t command {};
            if (auto result = read_pipe_data(pipe.native_handle(), &command, sizeof(command));
                !result) {
                return std::unexpected(result.error());
            }

            const auto action = impl_->commands.find(command);
            if (action == impl_->commands.end()) {
                return std::unexpected(Error { ERROR_INVALID_DATA });
            }

            action->second();
        }

        return {};
    }

    Result<void> send_string(Pipe& pipe, std::string_view message) {
        if (message.size() > maximum_message_size) {
            return std::unexpected(Error { ERROR_BAD_LENGTH });
        }

        const auto message_size = static_cast<std::uint32_t>(message.size());
        if (auto result = write_pipe_data(pipe.native_handle(), &message_size, sizeof(message_size));
            !result) {
            return result;
        }

        return write_pipe_data(pipe.native_handle(), message.data(), message.size());
    }

    Result<std::string> read_string(Pipe& pipe) {
        std::uint32_t message_size {};
        if (auto result = read_pipe_data(pipe.native_handle(), &message_size, sizeof(message_size));
            !result) {
            return std::unexpected(result.error());
        }

        if (message_size > maximum_message_size) {
            return std::unexpected(Error { ERROR_BAD_LENGTH });
        }

        std::string message(message_size, '\0');
        if (auto result = read_pipe_data(pipe.native_handle(), message.data(), message.size());
            !result) {
            return std::unexpected(result.error());
        }

        return message;
    }

}
