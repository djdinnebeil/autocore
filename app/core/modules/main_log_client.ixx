/**
 * \file main_log_client.ixx
 * \brief Owns one component's asynchronous connection to logger_ac.exe.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.logging.client;

import std;
import auto_core.core.logging.protocol;

export namespace ac::logger {

    /**
     * \brief Receives a diagnostic when the central logger becomes unavailable.
     */
    using FailureHandler = std::function<void(std::string_view)>;

    /**
     * \brief Owns an asynchronous, bounded connection to `logger_ac.exe`.
     *
     * The worker connects to the local named pipe `auto_core_logger` with a
     * custom `CreateFileW` retry loop (200 ms, five-second window) so
     * `close()` can interrupt the wait. `pipes::connect_to_pipe_server` is not
     * used because that helper is not stoppable the same way.
     *
     * Events are queued while connecting. The queue holds at most 1,000 events
     * and 1 MiB of component/message bytes; oldest events are dropped when
     * necessary. A single event whose payload plus header exceeds
     * `ac::logging::maximum_encoded_size` is rejected without queuing.
     *
     * After a failed send, the worker reconnects once. If that window fails,
     * central logging is disabled for this object and the failure handler, or
     * stderr, receives a diagnostic. Local component logging continues.
     */
    class MainLogConnection {
    public:
        /** \brief Constructs an idle connection. Call `start()` before `send()`. */
        AC_API MainLogConnection();
        AC_API ~MainLogConnection() noexcept;

        MainLogConnection(const MainLogConnection&) = delete;
        MainLogConnection& operator=(const MainLogConnection&) = delete;
        MainLogConnection(MainLogConnection&&) = delete;
        MainLogConnection& operator=(MainLogConnection&&) = delete;

        /**
         * \brief Starts the connection worker once.
         * \param component_name The source name attached to diagnostics and
         * shutdown requests.
         * \param failure_handler Optional receiver for connection failures.
         *
         * Calls after the first start are ignored. Thread-creation failures
         * propagate and leave the object startable.
         */
        AC_API void start(
            std::string_view component_name,
            FailureHandler failure_handler
        );

        /**
         * \brief Queues an event for the central logger.
         * \return `true` when queued; `false` before start, after close or
         * disablement, or when the event alone exceeds the queue byte limit
         * or cannot encode within `ac::logging::maximum_encoded_size`.
         */
        [[nodiscard]]
        AC_API bool send(const ac::logging::Event& event);

        /**
         * \brief Queues a logger shutdown event for this component.
         * \return Whether the event was accepted by `send()`.
         */
        [[nodiscard]]
        AC_API bool request_logger_shutdown();

        /**
         * \brief Stops the worker, cancels blocking I/O, and clears the queue.
         *
         * Cancels in-flight synchronous I/O on the worker with
         * `CancelSynchronousIo`. This operation is idempotent. Closing an idle
         * object permanently closes it; a closed object cannot be started
         * again.
         */
        AC_API void close() noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace ac::logger
