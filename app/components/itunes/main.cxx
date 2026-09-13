/**
\file main.cxx
\brief Entry point and process-level support for the iTunes component.
*/
import std;

import auto_core.core.pipes;
import itunes_protocol;
import itunes_component;
import itunes_client;
import itunes_monitor;
import itunes_pipe;
import itunes_removal;
import command_registry;
import itunes_registry;

import <Windows.h>;

void update_itunes_component() {
    itunes_component.update_log_file();
}

void log_init() {
    itunes_component.connect_to_logger();
    itunes_component.logg_and_logg("itunes_ac.exe started");
}

void end_itunes() {
    itunes_component.logg("iTunes is shutting down");
}

int main(int argc, char* argv[]) {
    const auto registry = create_itunes_command_registry();
    log_init();
    ac_itunes.set_config();
    if (ac_itunes.auto_start && !ac_itunes.initialize_com()) {
        itunes_component.logg_and_print(
            "Unable to initialize iTunes automation after retrying."
        );
    }
    ac::pipes::Pipe ac_itunes_pipe;
    auto connection = ac::pipes::connect_to_pipe_server(
        std::wstring { ac::protocol::itunes::pipe_name }
    );

    if (connection) {
        ac_itunes_pipe = std::move(*connection);
        ac::pipes::CommandDispatcher dispatcher;
        bool protocol_failed = false;
        register_itunes_pipe_commands(
            dispatcher,
            ac_itunes_pipe,
            registry,
            {
                .shutdown = end_itunes,
                .play_pause = itunes_play_pause,
                .next_song = itunes_next_song,
                .print_songs = print_itunes_songs,
                .print_next_up = print_next_up_song_list,
                .update_component = update_itunes_component,
                .previous_song = itunes_prev_song,
                .stop_song = itunes_stop_song,
                .remove_song = remove_itunes_song,
                .unknown_named = [](const std::string_view name) {
                    itunes_component.logg_and_print(
                        "Unknown iTunes command: {}", name
                    );
                }
            },
            protocol_failed
        );
        if (const auto result = dispatcher.process(ac_itunes_pipe);
            !result) {
            itunes_component.logg_and_print(
                "iTunes pipe failed. Error: {}",
                result.error().system_error
            );
        }
        if (protocol_failed) return 1;
    }
    else {
        itunes_component.logg_and_print(
            "Failed to connect to iTunes pipe. Error: {}",
            connection.error().system_error
        );
    }

    ac_itunes.shutdown();
    itunes_component.logg_and_logg("itunes_ac.exe has ended");


    return 0;
}
