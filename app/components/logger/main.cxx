import std;
import auto_core.pipes;
import auto_core.logging.protocol;
import logger_state;
import log_init;
import main_log;

import <Windows.h>;

namespace {
    std::atomic_bool logger_shutdown_requested = false;
    std::atomic_uint active_logger_connections = 0;
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


    void process_logger_connection(ac::pipes::Pipe logger_pipe) {
        while (true) {
            const auto data = ac::pipes::read_string(logger_pipe);

            if (!data) {
                break;
            }

            const auto event = ac::logging::decode(*data);
            if (!event) {
                logger_component.logg("Invalid logger protocol message");
                break;
            }

            if (event->type == ac::logging::EventType::shutdown) {
                logger_shutdown_requested.store(true);
                wake_logger_server();
                break;
            }

            write_to_main_log(*event);
        }

        DisconnectNamedPipe(logger_pipe.native_handle());
        active_logger_connections.fetch_sub(1);
    }

}


int main() {
    log_init();

    while (!logger_shutdown_requested.load()) {
        auto pipe_result = ac::pipes::create_pipe_server(
            std::wstring {logger_pipe_name}
        );

        if (!pipe_result) {
            logger_component.logg_and_print(
                "Failed to create logger pipe. Error: {}",
                pipe_result.error().system_error
            );
            return 1;
        }

        ac::pipes::Pipe logger_pipe = std::move(*pipe_result);

        logger_component.logg(
            "Waiting for logger client connection..."
        );

        const BOOL connected =
            ConnectNamedPipe(
                logger_pipe.native_handle(),
                nullptr
            );

        if (!connected) {
            const DWORD error = GetLastError();

            if (error != ERROR_PIPE_CONNECTED) {

                std::string error_msg = std::format("Failed to connect logger client. Error: {}", error);

                logger_component.logg(error_msg);
                std::cerr << error_msg << std::endl;

                const ac::logging::Event connection_error {
                    .component = "logger",
                    .message = error_msg,
                    .newline = true
                 };

                write_to_main_log(connection_error);

                continue;
            }
        }

        if (logger_shutdown_requested.load()) {
            DisconnectNamedPipe(logger_pipe.native_handle());
            break;
        }

        logger_component.logg(
            "Logger client connected"
        );

        active_logger_connections.fetch_add(1);
        std::thread(
            process_logger_connection,
            std::move(logger_pipe)
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

    const ac::logging::Event terminated_event {
        .component = "logger",
        .message = "logger.exe has now terminated\n***",
        .newline = true
    };
    logger_component.logg("logger.exe has now terminated");
    write_to_main_log(terminated_event);
    return 0;
}
