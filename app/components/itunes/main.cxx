/**
\file main.cxx
\brief Entry point and process-level support for the iTunes component.
*/
import std;

import auto_core.pipes;
import itunes_protocol;
import itunes_x;
import itunes_c;
import itunes_t;
import slash_i;

import <Windows.h>;

void update_itunes_component() {
    itunes_component.update_log_file();
}

void log_init() {
    itunes_component.connect_to_logger();
    itunes_component.logg_and_logg("ac_itunes.exe started");
}

void end_iTunes() {
    itunes_component.logg("iTunes is shutting down");
}

void set_commands(ac::pipes::CommandDispatcher& dispatcher) {
    dispatcher.set_command(ac::protocol::itunes::to_wire(ac::protocol::itunes::Command::shutdown), [&dispatcher]() {
        end_iTunes();
        dispatcher.request_stop();
    });
    dispatcher.set_command(ac::protocol::itunes::to_wire(ac::protocol::itunes::Command::play_pause), iTunes_play_pause);
    dispatcher.set_command(ac::protocol::itunes::to_wire(ac::protocol::itunes::Command::next_song), iTunes_next_song);
    dispatcher.set_command(ac::protocol::itunes::to_wire(ac::protocol::itunes::Command::print_songs), print_iTunes_songs);
    dispatcher.set_command(ac::protocol::itunes::to_wire(ac::protocol::itunes::Command::print_next_up), print_next_up_song_list);
    dispatcher.set_command(ac::protocol::itunes::to_wire(ac::protocol::itunes::Command::update_component), update_itunes_component);
    dispatcher.set_command(ac::protocol::itunes::to_wire(ac::protocol::itunes::Command::previous_song), iTunes_prev_song);
    dispatcher.set_command(ac::protocol::itunes::to_wire(ac::protocol::itunes::Command::stop_song), iTunes_stop_song);
    dispatcher.set_command(ac::protocol::itunes::to_wire(ac::protocol::itunes::Command::remove_song), remove_iTunes_song);
}

int main() {
    log_init();
    ac::pipes::CommandDispatcher dispatcher;
    set_commands(dispatcher);
    ac::pipes::Pipe ac_itunes_pipe;
    auto connection = ac::pipes::connect_to_pipe_server(
        std::wstring { ac::protocol::itunes::pipe_name }
    );

    if (connection) {
        ac_itunes_pipe = std::move(*connection);
        if (const auto result = dispatcher.process(ac_itunes_pipe);
            !result) {
            itunes_component.logg_and_print(
                "iTunes pipe failed. Error: {}",
                result.error().system_error
            );
        }
    }
    else {
        itunes_component.logg_and_print(
            "Failed to connect to iTunes pipe. Error: {}",
            connection.error().system_error
        );
    }

    ac_iTunes.finalize_com();
    itunes_component.logg_and_logg("ac_itunes.exe has ended");


    return 0;
}
