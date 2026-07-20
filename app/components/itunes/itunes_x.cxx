/**
\file itunes_x.cxx
\brief Entry point and process-level support for the iTunes component.
*/
module itunes_x;
import visual;
import itunes_c;
import itunes_t;
import slash_i;
import logger;
import logger_c;
import pipes_x;
import <Windows.h>;

Logger iTunes_logger("itunes");

void update_iTunes_logger() {
    update_main_log_file();
    iTunes_logger.update_log_file();
}

void log_init() {
    update_main_log_file();
    iTunes_logger.logg_and_logg("ac_itunes.exe started");
}

wstring pipe_name = L"ac_itunes_pipe";

void end_iTunes() {
    iTunes_logger.logg("iTunes is shutting down");
    end_process = true;
}

void set_command_map() {
    command_map[0] = {[]() {end_iTunes();}};
    command_map[1] = {iTunes_play_pause};
    command_map[2] = {iTunes_next_song};
    command_map[3] = {print_iTunes_songs};
    command_map[4] = {print_next_up_song_list};
    command_map[5] = {update_iTunes_logger};
    command_map[6] = {iTunes_prev_song};
    command_map[7] = {iTunes_stop_song};
    command_map[9] = {remove_iTunes_song};
}

int main() {
    log_init();
    set_command_map();
    HANDLE ac_itunes_pipe = connect_to_pipe_server(pipe_name);
    if (ac_itunes_pipe != NULL) {
        iTunes_logger.logg_and_logg("connected to pipe '{}' server", pipe_name);
        process_pipe_commands(ac_itunes_pipe);
    }
    else {
        print("Failed to connect to pipe server.");
    }
    ac_iTunes.finalize_com();
    iTunes_logger.logg_and_logg("ac_itunes.exe has ended");
    iTunes_logger.close_log_file();
    CloseHandle(ac_itunes_pipe);
    close_main_log_file();
    return 0;
}
