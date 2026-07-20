/**
\file wake.cxx
\brief Logs system wake events, extending Windows' native logging to offer historical wake event tracking.

By polling system information, this module provides detailed logging of wake events,
aiding in the analysis of system behavior and Auto Core's interaction with the host machine.
*/
import visual;
import pipes_x;
import wake_logging;
import <Windows.h>;

wstring pipe_name = L"wake_pipe";

/**
 * \brief Ends the wake component.
 */
void end_wake() {
    wake_logger.logg("wake_logger is shutting down");
    end_process = true;
}

/**
 * \brief Sets up the command map for handling pipe commands.
 *
 * This function maps integer command IDs to corresponding functions that handle
 * specific commands for the iTunes component.
 */
void set_command_map() {
    command_map[0] = {[]() {  end_wake(); }};
    command_map[1] = {log_last_wake};
    command_map[2] = {update_wake_logger};
}

int main() {
    log_init();
    log_last_wake();
    set_command_map();
    HANDLE wake_pipe = connect_to_pipe_server(pipe_name);
    if (wake_pipe != NULL) {
        wake_logger.logg_and_logg("connected to pipe '{}' server", pipe_name);
        process_pipe_commands(wake_pipe);
    }
    else {
        wake_logger.logg_and_print("Failed to connect to pipe server.");
    }
    wake_logger.logg_and_logg("wake.exe has ended");
    wake_logger.close_log_file();
    CloseHandle(wake_pipe);
    close_main_log_file();
    return 0;
}
