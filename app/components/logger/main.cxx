import std;
import component;
import pipes;
import main_log;

import <Windows.h>;

ac::Component logger_component {"logger"};

namespace {
    std::atomic_bool logger_shutdown_requested = false;
    std::atomic_uint active_logger_connections = 0;
}

void log_init() {
    logger_component.logg_and_logg("logger.exe started");
}

void update_logger_component() {
    update_main_log_file();
    logger_component.update_log_file();
}

void end_logger() {
    logger_component.logg("logger.exe is shutting down");
    ac::pipes::end_process = true;
}

void set_command_map() {
    using ac::pipes::command_map;
    command_map[0] = {[]() {  end_logger(); }};
    command_map[1] = {update_logger_component};
}

namespace {

    constexpr std::wstring_view logger_pipe_name =
        L"auto_core_logger";

    void wake_logger_server() {
        const std::wstring full_pipe_name =
            LR"(\\.\pipe\)" + std::wstring {logger_pipe_name};

        for (int attempt = 0; attempt < 20; ++attempt) {
            HANDLE wake_pipe = CreateFileW(
                full_pipe_name.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr
            );

            if (wake_pipe != INVALID_HANDLE_VALUE) {
                CloseHandle(wake_pipe);
                return;
            }

            Sleep(10);
        }
    }


    void process_logger_connection(
        HANDLE logger_pipe,
        ac::Component& logger_component
    ) {
        while (true) {
            auto event =
                ac::pipes::read_log_event(logger_pipe);

            if (!event) {
                break;
            }

            if (event->type == ac::pipes::LogEventType::shutdown) {
                logger_shutdown_requested.store(true);
                wake_logger_server();
                break;
            }

            write_to_main_log(*event);
        }

        DisconnectNamedPipe(logger_pipe);
        CloseHandle(logger_pipe);
        active_logger_connections.fetch_sub(1);
    }

}


int main() {
    log_init();
    set_command_map();

    update_main_log_file();

    while (!logger_shutdown_requested.load()) {
        HANDLE logger_pipe =
            ac::pipes::create_pipe_server(
                std::wstring {logger_pipe_name},
                logger_component
            );

        if (logger_pipe == INVALID_HANDLE_VALUE) {
            return 1;
        }

        logger_component.logg(
            "Waiting for logger client connection..."
        );

        const BOOL connected =
            ConnectNamedPipe(
                logger_pipe,
                nullptr
            );

        if (!connected) {
            const DWORD error = GetLastError();

            if (error != ERROR_PIPE_CONNECTED) {
                logger_component.logg_and_print(
                    "Failed to connect logger client. Error: {}",
                    error
                );

                CloseHandle(logger_pipe);
                continue;
            }
        }

        if (logger_shutdown_requested.load()) {
            DisconnectNamedPipe(logger_pipe);
            CloseHandle(logger_pipe);
            break;
        }

        logger_component.logg(
            "Logger client connected"
        );

        active_logger_connections.fetch_add(1);
        std::thread(
            process_logger_connection,
            logger_pipe,
            std::ref(logger_component)
        ).detach();
    }

    // Component shutdown signals are sent before Auto Core's final logger
    // signal. Give their already-connected logging clients time to flush and
    // disconnect so logger.exe remains the last component to finish.
    const ULONGLONG connection_wait_deadline = GetTickCount64() + 5000;
    while (
        active_logger_connections.load() != 0 &&
        GetTickCount64() < connection_wait_deadline
    ) {
        Sleep(10);
    }

    const ac::pipes::LogEvent terminated_event {
        .component = "logger",
        .message = "logger.exe has now terminated\n***",
        .newline = true
    };
    write_to_main_log(terminated_event);
    return 0;
}
