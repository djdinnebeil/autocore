/**
\file main.cxx
\brief Logs system wake events, extending Windows' native logging to offer historical wake event tracking.

By polling system information, this module provides detailed logging of wake events,
aiding in the analysis of system behavior and Auto Core's interaction with the host machine.
*/
import std;
import logger;
import logger_x;
import pipes;
import wake_logging;

import <Windows.h>;

std::wstring pipe_name = L"wake_pipe";

/**
 * \brief Ends the wake component.
 */
void end_wake() {
    wake_component.logg("wake.exe is shutting down");
    ac::pipes::end_process = true;
}

/**
 * \brief Sets up the command map for handling pipe commands.
 *
 * This function maps integer command IDs to corresponding functions that handle
 * specific commands for the iTunes component.
 */
void set_command_map() {
    using ac::pipes::command_map;
    command_map[0] = {[]() {  end_wake(); }};
    command_map[1] = {log_last_wake};
    command_map[2] = {update_wake_component};
}

int main() {
    log_init();
    log_last_wake();
    set_command_map();
    HANDLE wake_pipe = ac::pipes::connect_to_pipe_server(pipe_name, wake_component);
    if (wake_pipe != NULL) {
        wake_component.logg_and_logg("connected to pipe '{}' server", pipe_name);
        ac::pipes::process_pipe_commands(wake_pipe, wake_component);
    }
    else {
        wake_component.logg_and_print("Failed to connect to pipe server.");
    }
    wake_component.logg_and_logg("wake.exe has ended");
    ac::logger::close_logger_connection();
    CloseHandle(wake_pipe);
    return 0;
}
