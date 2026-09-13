module spotify_pipe;

import std;

import auto_core.core.pipes;
import command_registry;
import spotify_protocol;

void register_spotify_pipe_commands(
    ac::pipes::CommandDispatcher& dispatcher,
    ac::pipes::Pipe& pipe,
    const command_registry::Registry& registry,
    spotify_pipe_actions actions,
    bool& protocol_failed
) {
    using ac::protocol::spotify::Command;
    using ac::protocol::spotify::to_wire;

    dispatcher.set_command(to_wire(Command::shutdown),
        [&dispatcher, action = std::move(actions.shutdown)] {
            action();
            dispatcher.request_stop();
        });
    dispatcher.set_command(to_wire(Command::play_pause), std::move(actions.play_pause));
    dispatcher.set_command(to_wire(Command::next_song), std::move(actions.next_song));
    dispatcher.set_command(to_wire(Command::print_songs), std::move(actions.print_songs));
    dispatcher.set_command(to_wire(Command::get_queue), std::move(actions.get_queue));
    dispatcher.set_command(to_wire(Command::update_component), std::move(actions.update_component));
    dispatcher.set_command(to_wire(Command::switch_player), std::move(actions.switch_player));
    dispatcher.set_command(to_wire(Command::download_album_cover), std::move(actions.download_album_cover));
    dispatcher.set_command(to_wire(Command::invoke_named),
        [&dispatcher, &pipe, &registry, &protocol_failed,
            report_unknown = std::move(actions.unknown_named)] {
            const auto name = ac::pipes::read_string(pipe);
            if (!name) {
                protocol_failed = true;
                dispatcher.request_stop();
                return;
            }

            if (auto action = registry.resolve(*name)) {
                action();
            }
            else if (report_unknown) {
                report_unknown(*name);
            }
        });
}
