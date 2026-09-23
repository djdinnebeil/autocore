module itunes_pipe;

import std;

import auto_core.core.pipes;
import command_registry;
import component_protocol;

void register_itunes_pipe_commands(
    ac::pipes::CommandDispatcher& dispatcher,
    ac::pipes::Pipe& pipe,
    const command_registry::Registry& registry,
    itunes_pipe_actions actions,
    bool& protocol_failed
) {
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::shutdown
        ),
        [&dispatcher, action = std::move(actions.shutdown)] {
            action();
            dispatcher.request_stop();
        }
    );
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::invoke
        ),
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
        }
    );
}
