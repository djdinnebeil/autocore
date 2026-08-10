module;

#include "ac_api.hpp"

export module pipes;

import std;
import component;
import <Windows.h>;

export namespace ac::pipes {

    AC_API extern std::unordered_map<int, std::function<void()>> command_map;
    AC_API extern bool end_process;

    AC_API HANDLE create_pipe_server(
        const std::wstring& pipe_name,
        ac::Component& component
    );

    AC_API HANDLE connect_to_pipe_server(
        const std::wstring& pipe_name,
        ac::Component& component
    );

    AC_API bool send_pipe_command(
        HANDLE h_pipe,
        int command
    );

    AC_API void process_pipe_commands(
        HANDLE h_pipe,
        ac::Component& component
    );

    AC_API bool send_string(
        HANDLE h_pipe,
        std::string_view message
    );

    AC_API std::optional<std::string> read_string(
        HANDLE h_pipe
    );

    enum class LogEventType {
        message,
        shutdown
    };

    struct LogEvent {
        LogEventType type = LogEventType::message;
        std::string component;
        std::string message;
        bool newline;
    };

    AC_API bool send_log_event(
        HANDLE h_pipe,
        const LogEvent& event
    );

    AC_API std::optional<LogEvent> read_log_event(
        HANDLE h_pipe
    );

}
