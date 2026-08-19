/**
 * \file pipes.ixx
 * \brief Provides owning Win32 named-pipe handles and simple pipe protocols.
 *
 * Pipe operations report Win32 failure codes through `Result` instead of
 * throwing. Strings use a 32-bit byte-count prefix and are limited to 1 MiB.
 * Commands are transferred as 32-bit integer values.
 */
module;

#include "ac_api.hpp"

export module auto_core.pipes;

import std;

export namespace ac::pipes {

    /**
     * \brief Describes a failed pipe operation.
     */
    struct Error {
        /** \brief The Win32 system error code for the failure. */
        unsigned long system_error {};
    };

    /**
     * \brief A pipe result containing either a value or a Win32 error.
     */
    template<typename T>
    using Result = std::expected<T, Error>;

    /**
     * \brief Owns a Win32 pipe handle.
     *
     * A `Pipe` closes its handle when reset, move-assigned, or destroyed.
     * Ownership can be transferred with move operations or `release()`.
     */
    class Pipe {
    public:
        using native_handle_type = void*;

        /** \brief Constructs an invalid pipe. */
        AC_API Pipe() noexcept;

        /**
         * \brief Adopts ownership of a native handle.
         * \param handle The handle to own, or an invalid handle value.
         */
        AC_API explicit Pipe(native_handle_type handle) noexcept;
        AC_API ~Pipe() noexcept;

        Pipe(const Pipe&) = delete;
        Pipe& operator=(const Pipe&) = delete;

        AC_API Pipe(Pipe&& other) noexcept;
        AC_API Pipe& operator=(Pipe&& other) noexcept;

        /** \brief Returns the owned native handle without releasing it. */
        [[nodiscard]] AC_API native_handle_type native_handle() const noexcept;

        /** \brief Returns whether the pipe owns a usable handle. */
        [[nodiscard]] AC_API bool valid() const noexcept;

        /**
         * \brief Requests cancellation of pending I/O on the pipe.
         * \return `true` if Windows accepts the cancellation request.
         */
        AC_API bool cancel() noexcept;

        /**
         * \brief Releases ownership without closing the handle.
         * \return The previously owned handle.
         */
        AC_API native_handle_type release() noexcept;

        /** \brief Closes the owned handle and makes the pipe invalid. */
        AC_API void reset() noexcept;

        /**
         * \brief Closes the owned handle and adopts a replacement.
         * \param handle The replacement handle to own.
         */
        AC_API void reset(native_handle_type handle) noexcept;

    private:
        native_handle_type handle_;
    };

    /**
     * \brief Reads integer commands from a pipe and invokes registered actions.
     */
    class CommandDispatcher {
    public:
        AC_API CommandDispatcher();
        AC_API ~CommandDispatcher();

        CommandDispatcher(const CommandDispatcher&) = delete;
        CommandDispatcher& operator=(const CommandDispatcher&) = delete;
        CommandDispatcher(CommandDispatcher&&) = delete;
        CommandDispatcher& operator=(CommandDispatcher&&) = delete;

        /**
         * \brief Registers or replaces the action for a command value.
         * \param command The command value received from the pipe.
         * \param action The action to invoke when the command is received.
         */
        AC_API void set_command(
            int command,
            std::function<void()> action
        );

        /**
         * \brief Requests that processing stop after the current blocking pipe
         * operation or command finishes.
         *
         * This function may be called from another thread. It does not cancel
         * a ReadFile operation that is already in progress.
         */
        AC_API void request_stop() noexcept;

        [[nodiscard]]
        AC_API bool stop_requested() const noexcept;

        /**
         * \brief Processes commands until stopped or an operation fails.
         *
         * An unregistered command fails with `ERROR_INVALID_DATA`. Exceptions
         * thrown by a registered action propagate to the caller.
         *
         * \param pipe The connected pipe from which commands are read.
         * \return Success after a stop request, or the first pipe/protocol
         * error.
         */
        [[nodiscard]]
        AC_API Result<void> process(Pipe& pipe);

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

    /**
     * \brief Creates a duplex byte-mode named-pipe server instance.
     * \param pipe_name The local name, without the `\\.\pipe\` prefix.
     * \return An owning server handle, or the Win32 creation error.
     */
    AC_API Result<Pipe> create_pipe_server(
        const std::wstring& pipe_name
    );

    /**
     * \brief Connects to a local named-pipe server.
     *
     * Busy and not-yet-created servers are retried for up to five seconds.
     *
     * \param pipe_name The local name, without the `\\.\pipe\` prefix.
     * \return An owning client handle, or the final Win32 connection error.
     */
    AC_API Result<Pipe> connect_to_pipe_server(
        const std::wstring& pipe_name
    );

    /**
     * \brief Writes one 32-bit command value to a connected pipe.
     * \param pipe The destination pipe.
     * \param command The command value to send.
     */
    AC_API Result<void> send_pipe_command(
        Pipe& pipe,
        int command
    );

    /**
     * \brief Writes one length-prefixed byte string to a connected pipe.
     *
     * Empty strings and embedded null bytes are preserved. Messages larger
     * than 1 MiB fail with `ERROR_BAD_LENGTH` before data is written.
     *
     * \param pipe The destination pipe.
     * \param message The bytes to send.
     */
    AC_API Result<void> send_string(
        Pipe& pipe,
        std::string_view message
    );

    /**
     * \brief Reads one length-prefixed byte string from a connected pipe.
     *
     * A declared size larger than 1 MiB fails with `ERROR_BAD_LENGTH`.
     *
     * \param pipe The source pipe.
     * \return The received bytes, or the first pipe/protocol error.
     */
    AC_API Result<std::string> read_string(
        Pipe& pipe
    );

} // namespace ac::pipes
