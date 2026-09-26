#include "logger_session_detail.hpp"
#include "merge_detail.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#pragma comment(lib, "Shell32.lib")

import std;
import auto_core.core.component;
import auto_core.core.error;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.core.pipes;
import component_protocol;

namespace {

ac::Component logger_component {"logger"};

void report_merge(const ac::logger::detail::MergeResult& result) {
    if (!result.ok) {
        ac::error::log(result.message);
        logger_component.log_print("{}", result.message);
    }
}

void log_periodic_merge(const ac::logger::detail::MergeResult& result) {
    report_merge(result);
    if (result.ok) {
        logger_component.log(
            "Periodic merge ({}s)",
            ac::logging::config::merge_interval_seconds()
        );
    }
}

[[nodiscard]]
int run_once(const bool shutdown_launch) {
    if (shutdown_launch) {
        logger_component.log_main("Shutdown one-shot merge");
    }
    else {
        logger_component.log("One-shot merge");
    }
    const auto result = ac::logger::detail::merge_once(
        ac::logging::config::directory()
    );
    report_merge(result);
    return result.ok ? 0 : 1;
}

[[nodiscard]]
int run_hosted(ac::pipes::Pipe pipe) {
    ac::pipes::CommandDispatcher dispatcher;
    struct Event {
        HANDLE handle = nullptr;
        ~Event() {
            if (handle != nullptr) {
                CloseHandle(handle);
            }
        }
    } shutdown_event {CreateEventW(nullptr, TRUE, FALSE, nullptr)};
    if (shutdown_event.handle == nullptr) {
        logger_component.log_print(
            "Failed to create the logger shutdown event. Error: {}",
            GetLastError()
        );
        return 1;
    }

    const auto schedule = ac::logger::detail::schedule_for(
        ac::logging::config::merge_interval_seconds()
    );
    const auto interval_ms = schedule.wait_forever
        ? INFINITE
        : static_cast<DWORD>(ac::logger::detail::interval_milliseconds(
            ac::logging::config::merge_interval_seconds()
        ));

    std::jthread worker([handle = shutdown_event.handle, schedule, interval_ms] {
        const auto directory = ac::logging::config::directory();
        if (schedule.merge_before_wait) {
            log_periodic_merge(ac::logger::detail::merge_once(directory));
        }
        while (true) {
            const DWORD wake = WaitForSingleObject(handle, interval_ms);
            const auto outcome = ac::logger::detail::classify_wait(wake);
            if (outcome == ac::logger::detail::WaitOutcome::interval) {
                log_periodic_merge(ac::logger::detail::merge_once(directory));
                continue;
            }
            if (outcome == ac::logger::detail::WaitOutcome::failed) {
                logger_component.log_print(
                    "Logger wait failed. Error: {}",
                    GetLastError()
                );
            }
            else if (outcome == ac::logger::detail::WaitOutcome::unexpected) {
                logger_component.log_print(
                    "Unexpected logger wait result: {}",
                    wake
                );
            }
            break;
        }
    });

    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::shutdown
        ),
        [&dispatcher, handle = shutdown_event.handle] {
            logger_component.log_main("shutdown signal received");
            SetEvent(handle);
            dispatcher.request_stop();
        }
    );
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::invoke
        ),
        [&pipe, &dispatcher] {
            const auto expression = ac::pipes::read_string(pipe);
            if (!expression) {
                dispatcher.request_stop();
                return;
            }
            logger_component.log_print(
                "Unknown logger command: {}",
                *expression
            );
        }
    );

    if (const auto hello = ac::pipes::send_string(
            pipe,
            ac::protocol::component::make_hello({})
        ); !hello) {
        SetEvent(shutdown_event.handle);
        logger_component.log_print(
            "Failed to send logger hello. Error: {}",
            hello.error().system_error
        );
        return 1;
    }

    if (const auto result = dispatcher.process(pipe); !result) {
        SetEvent(shutdown_event.handle);
        logger_component.log_print(
            "Logger pipe failed. Error: {}",
            result.error().system_error
        );
    }

    worker.join();
    logger_component.log_main("program terminated");
    return 0;
}

} // namespace

int main() {
    int argument_count = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
    bool once = false;
    bool shutdown_once = false;
    if (arguments != nullptr) {
        for (int index = 1; index < argument_count; ++index) {
            if (ac::logger::detail::is_once_argument(arguments[index])) {
                once = true;
            }
            else if (ac::logger::detail::is_shutdown_argument(arguments[index])) {
                shutdown_once = true;
            }
        }
        LocalFree(arguments);
    }
    if (once) {
        return run_once(shutdown_once);
    }

    const std::wstring pipe_name =
        LR"(\\.\pipe\)" + ac::protocol::component::pipe_name("logger");
    const HANDLE handle = CreateFileW(
        pipe_name.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );
    const unsigned long open_error = handle == INVALID_HANDLE_VALUE
        ? GetLastError()
        : 0;
    const auto kind = ac::logger::detail::classify_pipe_open(
        handle != INVALID_HANDLE_VALUE,
        open_error
    );

    if (kind == ac::logger::detail::LaunchKind::manual) {
        return run_once(false);
    }
    if (kind == ac::logger::detail::LaunchKind::failed) {
        logger_component.log_print(
            "Failed to connect to the logger pipe. Error: {}",
            open_error
        );
        return 1;
    }

    return run_hosted(ac::pipes::Pipe {handle});
}
