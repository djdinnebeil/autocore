/**
 * \file main_log_client.ixx
 * \brief Owns one component's asynchronous connection to logger.exe.
 */
module;

#include "ac_api.hpp"

export module auto_core.logging.client;

import std;
import auto_core.logging.protocol;

export namespace ac::logger {

    /**
     * \brief Receives a diagnostic when the central logger becomes unavailable.
     */
    using FailureHandler = std::function<void(std::string_view)>;

    /**
     * \brief Owns an asynchronous, bounded connection to `logger.exe`.
     *
     * Events are queued while a worker connects to the local logger pipe.
     * The queue holds at most 1,000 events and 1 MiB of component/message
     * bytes; oldest events are dropped when necessary. Connection attempts
     * use a five-second window. After failure, central logging is disabled for
     * this object and the failure handler, or stderr, receives a diagnostic.
     */
    class MainLogConnection {
    public:
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
         * disablement, or when the event alone exceeds the byte limit.
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
         * This operation is idempotent. Closing an idle object permanently
         * closes it; a closed object cannot be started again.
         */
        AC_API void close() noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace ac::logger
