module pipes;

import std;
import component;

import <Windows.h>;

namespace {

    constexpr std::uint32_t max_log_event_size = 1024 * 1024;

    bool write_pipe_data(
        HANDLE h_pipe,
        const void* data,
        DWORD size
    ) {
        DWORD bytes_written {};

        return WriteFile(
            h_pipe,
            data,
            size,
            &bytes_written,
            nullptr
        ) &&
            bytes_written == size;
    }


    bool read_pipe_data(
        HANDLE h_pipe,
        void* data,
        DWORD size
    ) {
        DWORD bytes_read {};

        return ReadFile(
            h_pipe,
            data,
            size,
            &bytes_read,
            nullptr
        ) &&
            bytes_read == size;
    }

}


namespace ac::pipes {

    std::unordered_map<int, std::function<void()>> command_map {};
    bool end_process {};

    HANDLE create_pipe_server(
        const std::wstring& pipe_name,
        ac::Component& component
    ) {
        const std::wstring full_pipe_name =
            LR"(\\.\pipe\)" + pipe_name;

        HANDLE h_pipe = CreateNamedPipeW(
            full_pipe_name.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE |
            PIPE_READMODE_BYTE |
            PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            4096,
            4096,
            0,
            nullptr
        );

        if (h_pipe == INVALID_HANDLE_VALUE) {
            component.logg_and_print(
                "Failed to create pipe '{}'. Error: {}",
                pipe_name,
                GetLastError()
            );

            return INVALID_HANDLE_VALUE;
        }

        component.logg_and_logg(
            "Pipe '{}' created",
            pipe_name
        );

        return h_pipe;
    }


    HANDLE connect_to_pipe_server(
        const std::wstring& pipe_name,
        ac::Component& component
    ) {
        const std::wstring full_pipe_name =
            LR"(\\.\pipe\)" + pipe_name;

        HANDLE h_pipe = CreateFileW(
            full_pipe_name.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (h_pipe == INVALID_HANDLE_VALUE) {
            component.logg_and_print(
                "Failed to connect to pipe '{}'. Error: {}",
                pipe_name,
                GetLastError()
            );

            return INVALID_HANDLE_VALUE;
        }

        component.logg_and_logg(
            "Connected to pipe '{}'",
            pipe_name
        );

        return h_pipe;
    }


    bool send_pipe_command(
        HANDLE h_pipe,
        int command
    ) {
        return write_pipe_data(
            h_pipe,
            &command,
            sizeof(command)
        );
    }


    void process_pipe_commands(
        HANDLE h_pipe,
        ac::Component& component
    ) {
        while (!end_process) {
            std::int32_t command {};

            if (!read_pipe_data(
                h_pipe,
                &command,
                sizeof(command)
            )) {

                component.logg_and_print(
                    "Failed to read pipe command. Error: {}",
                    GetLastError()
                );

                break;
            }

            auto action = command_map.find(command);

            if (action == command_map.end()) {
                component.logg_and_print(
                    "Invalid command received: {}",
                    command
                );

                continue;
            }

            action->second();
        }
    }


    bool send_string(
        HANDLE h_pipe,
        std::string_view message
    ) {
        if (message.size() >
            (std::numeric_limits<std::uint32_t>::max)()) {

            return false;
        }

        const auto message_size =
            static_cast<std::uint32_t>(message.size());

        if (!write_pipe_data(
            h_pipe,
            &message_size,
            sizeof(message_size)
        )) {

            return false;
        }

        if (message.empty()) {
            return true;
        }

        return write_pipe_data(
            h_pipe,
            message.data(),
            message_size
        );
    }


    std::optional<std::string> read_string(
        HANDLE h_pipe
    ) {
        std::uint32_t message_size {};

        if (!read_pipe_data(
            h_pipe,
            &message_size,
            sizeof(message_size)
        )) {

            return std::nullopt;
        }

        if (message_size > max_log_event_size) {
            return std::nullopt;
        }

        std::string message(
            message_size,
            '\0'
        );

        if (message.empty()) {
            return message;
        }

        if (!read_pipe_data(
            h_pipe,
            message.data(),
            message_size
        )) {

            return std::nullopt;
        }

        return message;
    }


    bool send_log_event(
        HANDLE h_pipe,
        const LogEvent& event
    ) {
        const std::string data = std::format(
            "{}\n{}\n{}\n{}",
            static_cast<int>(event.type),
            event.component,
            event.newline ? 1 : 0,
            event.message
        );

        return send_string(h_pipe, data);
    }

    std::optional<LogEvent> read_log_event(
        HANDLE h_pipe
    ) {
        const auto data = read_string(h_pipe);

        if (!data) {
            return std::nullopt;
        }

        const std::size_t type_newline =
            data->find('\n');

        if (type_newline == std::string::npos) {
            return std::nullopt;
        }

        const std::size_t component_newline =
            data->find('\n', type_newline + 1);

        if (component_newline == std::string::npos) {
            return std::nullopt;
        }

        const std::size_t newline_newline =
            data->find('\n', component_newline + 1);

        if (newline_newline == std::string::npos) {
            return std::nullopt;
        }

        LogEvent event;

        if ((*data)[0] == '0') {
            event.type = LogEventType::message;
        }
        else if ((*data)[0] == '1') {
            event.type = LogEventType::shutdown;
        }
        else {
            return std::nullopt;
        }

        event.component =
            data->substr(type_newline + 1, component_newline - type_newline - 1);

        event.newline =
            (*data)[component_newline + 1] == '1';

        event.message =
            data->substr(newline_newline + 1);

        return event;
    }

}
