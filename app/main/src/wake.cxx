module wake;

import std;
import pipes;
import journey;
import ac_component;
import <Windows.h>;


namespace {
    HANDLE wake_pipe = INVALID_HANDLE_VALUE;
}

void create_wake_pipe() {
    wake_pipe = ac::pipes::create_pipe_server(L"wake_pipe", auto_core);
}

/**
 * \brief Starts the wake component executable.
 *
 * This function launches the wake component executable, `wake.exe`, which is responsible for logging wake events.
 */
void start_wake_component() {
    std::wstring wake_path = LR"(.\wake.exe)";
    ac::main::create_process(wake_path);
}

/**
 * \brief Sends a signal to terminate the wake component.
 *
 * This function sends a command to the wake component to gracefully shut down and terminate the process.
 */
void send_wake_end_signal() {
    ac::pipes::send_pipe_command(wake_pipe, 0);
}

/**
 * \brief Sends a signal to log the last wake event.
 *
 * This function sends a command to log the last wake event.
 */
void send_logg_wake_signal() {
    ac::pipes::send_pipe_command(wake_pipe, 1);
}

/**
 * \brief Updates the wake logger.
 *
 * This function sends a command to the wake component to update the log file with the latest information.
 */
void update_wake_logger() {
    ac::pipes::send_pipe_command(wake_pipe, 2);
}
