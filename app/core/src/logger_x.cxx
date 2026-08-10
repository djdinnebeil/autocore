module logger_x;

import std;
import pipes;
import component;
import <Windows.h>;

namespace {

    HANDLE logger_pipe = INVALID_HANDLE_VALUE;
    std::mutex logger_pipe_mutex;
}

namespace ac::logger {

    constexpr std::wstring_view logger_pipe_name =
        L"auto_core_logger";

    void connect_to_logger(ac::Component& component) {
        // connect_to_pipe_server() logs through the supplied component after
        // connecting. Do not hold logger_pipe_mutex during that call: the log
        // re-enters send_to_logger(), which uses the same mutex.
        HANDLE connected_pipe =
            ac::pipes::connect_to_pipe_server(
                std::wstring {logger_pipe_name},
                component
            );

        std::scoped_lock lock(logger_pipe_mutex);
        logger_pipe = connected_pipe;
    }

    void close_logger_connection() {
        std::scoped_lock lock(logger_pipe_mutex);
        if (logger_pipe != INVALID_HANDLE_VALUE) {
            CloseHandle(logger_pipe);
            logger_pipe = INVALID_HANDLE_VALUE;
        }
    }

    bool send_to_logger(
        const ac::pipes::LogEvent& event
    ) {
        std::scoped_lock lock(logger_pipe_mutex);
        if (logger_pipe == INVALID_HANDLE_VALUE) {
            return false;
        }

        return ac::pipes::send_log_event(
            logger_pipe,
            event
        );
    }

    bool shutdown_logger() {
        const ac::pipes::LogEvent event {
            .type = ac::pipes::LogEventType::shutdown,
            .component = "Auto Core",
            .message = {},
            .newline = true
        };

        return send_to_logger(event);
    }
}
