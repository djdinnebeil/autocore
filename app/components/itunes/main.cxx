/**
\file main.cxx
\brief Entry point and process-level support for the iTunes component.
*/
import std;

import print;
import logger;
import logger_x;
import pipes;
import itunes_x;
import itunes_c;
import itunes_t;
import slash_i;

import logger_x;

import <Windows.h>;

void update_itunes_component() {
    itunes_component.update_log_file();
}

void log_init() {
    ac::logger::connect_to_logger(itunes_component);
    itunes_component.logg_and_logg("ac_itunes.exe started");
}

std::wstring pipe_name = L"ac_itunes_pipe";

void end_iTunes() {
    itunes_component.logg("iTunes is shutting down");
    ac::pipes::end_process = true;
}

void set_command_map() {
    using ac::pipes::command_map;
    command_map[0] = {[]() {end_iTunes();}};
    command_map[1] = {iTunes_play_pause};
    command_map[2] = {iTunes_next_song};
    command_map[3] = {print_iTunes_songs};
    command_map[4] = {print_next_up_song_list};
    command_map[5] = {update_itunes_component};
    command_map[6] = {iTunes_prev_song};
    command_map[7] = {iTunes_stop_song};
    command_map[9] = {remove_iTunes_song};
}

int main() {
    log_init();
    set_command_map();
    HANDLE ac_itunes_pipe = ac::pipes::connect_to_pipe_server(pipe_name, itunes_component);
    if (ac_itunes_pipe != NULL) {
        itunes_component.logg_and_logg("connected to pipe '{}' server", pipe_name);
        ac::pipes::process_pipe_commands(ac_itunes_pipe, itunes_component);
    }
    else {
        itunes_component.logg_and_print("Failed to connect to pipe server.");
    }
    ac_iTunes.finalize_com();
    itunes_component.logg_and_logg("ac_itunes.exe has ended");
    ac::logger::close_logger_connection();
    CloseHandle(ac_itunes_pipe);
    return 0;
}
