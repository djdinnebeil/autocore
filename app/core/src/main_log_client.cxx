module auto_core.core.logging.client;

import std;
import auto_core.core.clock;
import auto_core.core.encoding;
import auto_core.core.pipes;
import auto_core.core.logging.protocol;
import <Windows.h>;

namespace {

    constexpr std::wstring_view logger_pipe_name = L"auto_core_logger";
    constexpr std::size_t max_queued_events = 1000;
    constexpr std::size_t max_queued_bytes = 1024 * 1024;
    constexpr std::size_t max_encodable_payload =
        ac::logging::maximum_encoded_size - ac::logging::encoded_header_size;
    constexpr auto connection_window = std::chrono::seconds(5);
    constexpr auto connection_retry = std::chrono::milliseconds(200);

    enum class ConnectionState {
        idle,
        connecting,
        connected,
        disabled,
        stopping,
        closed
    };

    std::size_t event_size(const ac::logging::Event& event) {
        return event.timestamp.size() + event.component.size() +
            event.message.size();
    }

    std::string executable_name() {
        std::wstring path(32768, L'\0');
        const DWORD length = GetModuleFileNameW(
            nullptr,
            path.data(),
            static_cast<DWORD>(path.size())
        );

        if (length == 0 || length == path.size()) {
            return "Component";
        }

        path.resize(length);
        return ac::encoding::to_utf8(
            std::filesystem::path {path}.filename().wstring()
        );
    }

}

namespace ac::logger {

    class MainLogConnection::Impl {
    public:
        std::mutex mutex;
        std::condition_variable cv;
        std::deque<ac::logging::Event> queue;
        std::size_t queued_bytes = 0;
        std::size_t dropped_events = 0;
        std::size_t dropped_bytes = 0;
        ConnectionState state = ConnectionState::idle;
        bool stop_requested = false;
        std::string component_name;
        FailureHandler failure_handler;
        std::thread worker;
        ac::pipes::Pipe live_pipe;

        bool wait_for_retry(
            std::unique_lock<std::mutex>& lock,
            const std::chrono::steady_clock::time_point deadline
        ) {
            return cv.wait_until(
                lock,
                (std::min)(
                    deadline,
                    std::chrono::steady_clock::now() + connection_retry
                ),
                [this]() { return stop_requested; }
            );
        }

        ac::pipes::Pipe connect_during_window() {
            const std::wstring full_pipe_name =
                LR"(\\.\pipe\)" + std::wstring {logger_pipe_name};
            const auto deadline =
                std::chrono::steady_clock::now() + connection_window;

            while (true) {
                {
                    std::scoped_lock lock(mutex);
                    if (stop_requested) {
                        return {};
                    }
                }

                HANDLE pipe = CreateFileW(
                    full_pipe_name.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    nullptr,
                    OPEN_EXISTING,
                    0,
                    nullptr
                );

                if (pipe != INVALID_HANDLE_VALUE) {
                    return ac::pipes::Pipe {pipe};
                }

                const DWORD error = GetLastError();
                if (
                    error != ERROR_PIPE_BUSY &&
                    error != ERROR_FILE_NOT_FOUND
                ) {
                    return {};
                }

                if (std::chrono::steady_clock::now() >= deadline) {
                    return {};
                }

                std::unique_lock lock(mutex);
                if (wait_for_retry(lock, deadline)) {
                    return {};
                }
            }
        }

        void report_logger_unavailable(
            const std::size_t discarded_events,
            const std::size_t discarded_bytes
        ) {
            std::string message = std::format(
                "{} (component '{}') failed to connect to logger_ac.exe within "
                "5 seconds. Central logging is disabled; component logging "
                "will continue.",
                executable_name(),
                component_name
            );

            if (discarded_events != 0) {
                message += std::format(
                    " {} central-log events ({} bytes) were discarded.",
                    discarded_events,
                    discarded_bytes
                );
            }

            if (failure_handler) {
                try {
                    failure_handler(message);
                    return;
                }
                catch (...) {
                }
            }

            std::cerr << message << '\n';
        }

        void disable() {
            std::size_t discarded_events = 0;
            std::size_t discarded_bytes = 0;

            {
                std::scoped_lock lock(mutex);

                for (const auto& event : queue) {
                    dropped_bytes += event_size(event);
                }

                dropped_events += queue.size();
                discarded_events = dropped_events;
                discarded_bytes = dropped_bytes;
                queue.clear();
                queued_bytes = 0;
                state = ConnectionState::disabled;
            }

            report_logger_unavailable(
                discarded_events,
                discarded_bytes
            );
        }

        bool adopt_pipe(ac::pipes::Pipe pipe) {
            std::scoped_lock lock(mutex);
            if (stop_requested) {
                return false;
            }
            live_pipe = std::move(pipe);
            state = ConnectionState::connected;
            return true;
        }

        bool write_encoded(const std::string_view encoded) {
            HANDLE handle = INVALID_HANDLE_VALUE;
            {
                std::scoped_lock lock(mutex);
                handle = live_pipe.native_handle();
            }
            if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
                return false;
            }

            ac::pipes::Pipe borrowed {handle};
            const auto result = ac::pipes::send_string(borrowed, encoded);
            borrowed.release();
            return static_cast<bool>(result);
        }

        void close_live_pipe() noexcept {
            ac::pipes::Pipe closing;
            {
                std::scoped_lock lock(mutex);
                (void)live_pipe.cancel();
                closing = std::move(live_pipe);
            }
        }

        void run() {
            ac::pipes::Pipe pipe = connect_during_window();

            if (!pipe.valid()) {
                std::scoped_lock lock(mutex);
                if (stop_requested) {
                    state = ConnectionState::closed;
                    return;
                }
            }

            if (!pipe.valid()) {
                disable();
                return;
            }

            if (!adopt_pipe(std::move(pipe))) {
                std::scoped_lock lock(mutex);
                state = ConnectionState::closed;
                return;
            }

            while (true) {
                ac::logging::Event event;
                std::size_t dropped_event_count = 0;
                std::size_t dropped_byte_count = 0;

                {
                    std::unique_lock lock(mutex);
                    cv.wait(lock, [this]() {
                        return stop_requested || !queue.empty();
                    });

                    if (queue.empty() && stop_requested) {
                        break;
                    }

                    event = std::move(queue.front());
                    queued_bytes -= event_size(event);
                    queue.pop_front();

                    dropped_event_count = dropped_events;
                    dropped_byte_count = dropped_bytes;
                    dropped_events = 0;
                    dropped_bytes = 0;
                }

                bool dropped_warning_sent =
                    dropped_event_count == 0;

                if (dropped_event_count != 0) {
                    const ac::logging::Event warning {
                        .timestamp = ac::clock::get_log_timestamp(),
                        .component = component_name,
                        .message = std::format(
                            "Central logging dropped {} events ({} bytes) "
                            "because its queue was full.",
                            dropped_event_count,
                            dropped_byte_count
                        ),
                        .newline = true
                    };

                    const auto encoded_warning =
                        ac::logging::encode(warning);

                    dropped_warning_sent = encoded_warning &&
                        write_encoded(*encoded_warning);
                }

                const auto encoded = ac::logging::encode(event);
                if (
                    dropped_warning_sent && encoded &&
                    write_encoded(*encoded)
                ) {
                    continue;
                }

                {
                    std::scoped_lock lock(mutex);
                    live_pipe.reset();
                    if (!dropped_warning_sent) {
                        dropped_events += dropped_event_count;
                        dropped_bytes += dropped_byte_count;
                    }
                    queue.push_front(std::move(event));
                    queued_bytes += event_size(queue.front());
                    state = ConnectionState::connecting;
                }

                pipe = connect_during_window();
                if (!pipe.valid()) {
                    std::scoped_lock lock(mutex);
                    if (stop_requested) {
                        queue.clear();
                        queued_bytes = 0;
                        state = ConnectionState::closed;
                        return;
                    }
                }

                if (!pipe.valid()) {
                    disable();
                    return;
                }

                if (!adopt_pipe(std::move(pipe))) {
                    std::scoped_lock lock(mutex);
                    queue.clear();
                    queued_bytes = 0;
                    state = ConnectionState::closed;
                    return;
                }
            }

            std::scoped_lock lock(mutex);
            live_pipe.reset();
            state = ConnectionState::closed;
        }
    };

    MainLogConnection::MainLogConnection()
        : impl_(std::make_unique<Impl>()) {
    }

    MainLogConnection::~MainLogConnection() noexcept {
        close();
    }

    void MainLogConnection::start(
        const std::string_view component_name,
        FailureHandler failure_handler
    ) {
        if (!impl_) {
            return;
        }

        std::scoped_lock lock(impl_->mutex);
        if (impl_->state != ConnectionState::idle) {
            return;
        }

        impl_->component_name = component_name;
        impl_->failure_handler = std::move(failure_handler);
        impl_->stop_requested = false;
        impl_->state = ConnectionState::connecting;

        try {
            impl_->worker = std::thread([impl = impl_.get()]() {
                impl->run();
            });
        }
        catch (...) {
            impl_->component_name.clear();
            impl_->failure_handler = {};
            impl_->state = ConnectionState::idle;
            throw;
        }
    }

    bool MainLogConnection::send(const ac::logging::Event& event) {
        if (!impl_) {
            return false;
        }

        std::scoped_lock lock(impl_->mutex);
        if (
            impl_->state != ConnectionState::connecting &&
            impl_->state != ConnectionState::connected
        ) {
            return false;
        }

        const std::size_t size = event_size(event);
        if (size > max_queued_bytes || size > max_encodable_payload) {
            ++impl_->dropped_events;
            impl_->dropped_bytes += size;
            return false;
        }

        while (
            !impl_->queue.empty() &&
            (
                impl_->queue.size() >= max_queued_events ||
                impl_->queued_bytes + size > max_queued_bytes
            )
        ) {
            const std::size_t dropped_size =
                event_size(impl_->queue.front());
            impl_->queued_bytes -= dropped_size;
            ++impl_->dropped_events;
            impl_->dropped_bytes += dropped_size;
            impl_->queue.pop_front();
        }

        impl_->queue.push_back(event);
        impl_->queued_bytes += size;
        impl_->cv.notify_one();
        return true;
    }

    bool MainLogConnection::request_logger_shutdown() {
        if (!impl_) {
            return false;
        }

        std::string component_name;

        {
            std::scoped_lock lock(impl_->mutex);
            component_name = impl_->component_name;
        }

        const ac::logging::Event event {
            .type = ac::logging::EventType::shutdown,
            .timestamp = ac::clock::get_log_timestamp(),
            .component = std::move(component_name),
            .message = {},
            .newline = true
        };

        return send(event);
    }

    void MainLogConnection::close() noexcept {
        if (!impl_) {
            return;
        }

        {
            std::scoped_lock lock(impl_->mutex);
            if (
                impl_->state == ConnectionState::closed &&
                !impl_->worker.joinable()
            ) {
                return;
            }

            impl_->stop_requested = true;
            if (impl_->state != ConnectionState::idle) {
                impl_->state = ConnectionState::stopping;
            }
        }

        impl_->cv.notify_all();
        impl_->close_live_pipe();

        if (impl_->worker.joinable()) {
            (void)CancelSynchronousIo(impl_->worker.native_handle());
            impl_->worker.join();
        }

        std::scoped_lock lock(impl_->mutex);
        impl_->queue.clear();
        impl_->queued_bytes = 0;
        impl_->dropped_events = 0;
        impl_->dropped_bytes = 0;
        impl_->failure_handler = {};
        impl_->state = ConnectionState::closed;
    }

}
