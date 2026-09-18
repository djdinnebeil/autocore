#include "catch_amalgamated.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#include <Windows.h>

import auto_core.core.pipes;
import command_registry;
import itunes_pipe;
import component_protocol;

namespace {

    struct connected_pipes {
        ac::pipes::Pipe server;
        ac::pipes::Pipe client;
    };

    std::wstring unique_pipe_name() {
        static std::atomic_uint counter {};
        return std::format(
            L"itunes_tests_{}_{}",
            GetCurrentProcessId(),
            counter.fetch_add(1, std::memory_order_relaxed)
        );
    }

    connected_pipes connect_pipe_pair() {
        const std::wstring name = unique_pipe_name();
        auto server = ac::pipes::create_pipe_server(name);
        if (!server) {
            throw std::system_error(
                static_cast<int>(server.error().system_error),
                std::system_category(),
                "Could not create iTunes test pipe"
            );
        }

        auto client = ac::pipes::connect_to_pipe_server(name);
        if (!client) {
            throw std::system_error(
                static_cast<int>(client.error().system_error),
                std::system_category(),
                "Could not connect iTunes test pipe"
            );
        }

        return {std::move(*server), std::move(*client)};
    }

    struct recorded_actions {
        std::vector<std::string> invoked;
        std::string unknown_name;

        command_registry::Action record(std::string name) {
            return [this, name = std::move(name)] {
                invoked.push_back(name);
            };
        }

        itunes_pipe_actions pipe_actions() {
            return {
                .shutdown = record("shutdown"),
                .play_pause = record("play_pause"),
                .next_song = record("next_song"),
                .print_songs = record("print_songs"),
                .print_next_up = record("print_next_up"),
                .update_component = record("update_component"),
                .previous_song = record("previous_song"),
                .stop_song = record("stop_song"),
                .remove_song = record("remove_song"),
                .unknown_named = [this](const std::string_view name) {
                    unknown_name = name;
                }
            };
        }
    };

    void send_command(ac::pipes::Pipe& pipe, ac::protocol::component::Request command) {
        REQUIRE(ac::pipes::send_pipe_command(
            pipe, ac::protocol::component::to_wire(command)
        ));
    }

} // namespace

TEST_CASE(
    "iTunes named commands dispatch their string payload over a named pipe",
    "[itunes][pipe][windows-integration]"
) {
    auto pipes = connect_pipe_pair();
    command_registry::Registry registry;
    recorded_actions actions;
    registry.add("itunes_test_named", actions.record("named"));
    ac::pipes::CommandDispatcher dispatcher;
    bool protocol_failed = false;
    register_itunes_pipe_commands(
        dispatcher, pipes.server, registry, actions.pipe_actions(), protocol_failed
    );

    send_command(pipes.client, ac::protocol::component::Request::invoke);
    REQUIRE(ac::pipes::send_string(pipes.client, "itunes_test_named"));
    send_command(pipes.client, ac::protocol::component::Request::shutdown);

    REQUIRE(dispatcher.process(pipes.server));
    CHECK_FALSE(protocol_failed);
    CHECK(actions.invoked == std::vector<std::string> {"named", "shutdown"});
}

TEST_CASE(
    "Unknown iTunes named commands are reported without stopping the pipe",
    "[itunes][pipe][windows-integration]"
) {
    auto pipes = connect_pipe_pair();
    command_registry::Registry registry;
    recorded_actions actions;
    ac::pipes::CommandDispatcher dispatcher;
    bool protocol_failed = false;
    register_itunes_pipe_commands(
        dispatcher, pipes.server, registry, actions.pipe_actions(), protocol_failed
    );

    send_command(pipes.client, ac::protocol::component::Request::invoke);
    REQUIRE(ac::pipes::send_string(pipes.client, "missing_itunes_command"));
    send_command(pipes.client, ac::protocol::component::Request::shutdown);

    REQUIRE(dispatcher.process(pipes.server));
    CHECK_FALSE(protocol_failed);
    CHECK(actions.unknown_name == "missing_itunes_command");
    CHECK(actions.invoked == std::vector<std::string> {"shutdown"});
}

TEST_CASE(
    "A missing iTunes named-command payload marks the protocol failed",
    "[itunes][pipe][windows-integration]"
) {
    auto pipes = connect_pipe_pair();
    command_registry::Registry registry;
    recorded_actions actions;
    ac::pipes::CommandDispatcher dispatcher;
    bool protocol_failed = false;
    register_itunes_pipe_commands(
        dispatcher, pipes.server, registry, actions.pipe_actions(), protocol_failed
    );

    send_command(pipes.client, ac::protocol::component::Request::invoke);
    pipes.client.reset();

    REQUIRE(dispatcher.process(pipes.server));
    CHECK(protocol_failed);
    CHECK(dispatcher.stop_requested());
    CHECK(actions.invoked.empty());
}

TEST_CASE(
    "Component hello carries termination policy without changing request IDs",
    "[component-protocol][unit]"
) {
    using ac::protocol::component::Request;
    using ac::protocol::component::TerminationPolicy;

    CHECK(ac::protocol::component::to_wire(Request::invoke) == 0);
    CHECK(ac::protocol::component::to_wire(Request::shutdown) == 1);

    const auto graceful = ac::protocol::component::parse_hello(
        ac::protocol::component::make_hello({"test_command"})
    );
    REQUIRE(graceful);
    CHECK(graceful->termination_policy == TerminationPolicy::graceful);
    REQUIRE(graceful->commands.size() == 1);
    CHECK(graceful->commands.front().name == "test_command");

    const auto force_allowed = ac::protocol::component::parse_hello(
        ac::protocol::component::make_hello(
            {},
            TerminationPolicy::force_allowed
        )
    );
    REQUIRE(force_allowed);
    CHECK(force_allowed->termination_policy ==
        TerminationPolicy::force_allowed);
}

TEST_CASE(
    "Missing or invalid termination policy defaults safely to graceful",
    "[component-protocol][unit]"
) {
    using ac::protocol::component::TerminationPolicy;

    const auto legacy = ac::protocol::component::parse_hello(
        "ac.component.v1\nlegacy_command\n"
    );
    REQUIRE(legacy);
    CHECK(legacy->termination_policy == TerminationPolicy::graceful);

    const auto invalid = ac::protocol::component::parse_hello(
        "ac.component.v1\ntermination_policy = unsafe\n"
    );
    REQUIRE(invalid);
    CHECK(invalid->termination_policy == TerminationPolicy::graceful);
    REQUIRE(invalid->skipped_lines.size() == 1);
}
