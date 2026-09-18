// Spotify local-pipe integration tests.
#include "catch_amalgamated.hpp"

#include <atomic>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#include <Windows.h>

import auto_core.core.pipes;
import command_registry;
import spotify_pipe;
import component_protocol;

namespace {

struct connected_pipes {
    ac::pipes::Pipe server;
    ac::pipes::Pipe client;
};

std::wstring unique_pipe_name() {
    static std::atomic_uint counter {};
    return std::format(
        L"spotify_tests_{}_{}",
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
            "Could not create Spotify test pipe"
        );
    }

    auto client = ac::pipes::connect_to_pipe_server(name);
    if (!client) {
        throw std::system_error(
            static_cast<int>(client.error().system_error),
            std::system_category(),
            "Could not connect Spotify test pipe"
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

    spotify_pipe_actions pipe_actions() {
        return {
            .shutdown = record("shutdown"),
            .play_pause = record("play_pause"),
            .next_song = record("next_song"),
            .print_songs = record("print_songs"),
            .get_queue = record("get_queue"),
            .update_component = record("update_component"),
            .switch_player = record("switch_player"),
            .download_album_cover = record("download_album_cover"),
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
    "Spotify named commands dispatch their string payload over a named pipe",
    "[spotify][pipe][windows-integration]"
) {
    auto pipes = connect_pipe_pair();
    command_registry::Registry registry;
    recorded_actions actions;
    registry.add("spotify_test_named", actions.record("named"));
    ac::pipes::CommandDispatcher dispatcher;
    bool protocol_failed = false;
    register_spotify_pipe_commands(
        dispatcher, pipes.server, registry, actions.pipe_actions(), protocol_failed
    );

    send_command(pipes.client, ac::protocol::component::Request::invoke);
    REQUIRE(ac::pipes::send_string(pipes.client, "spotify_test_named"));
    send_command(pipes.client, ac::protocol::component::Request::shutdown);

    REQUIRE(dispatcher.process(pipes.server));
    CHECK_FALSE(protocol_failed);
    CHECK(actions.invoked == std::vector<std::string> {"named", "shutdown"});
}

TEST_CASE(
    "Unknown Spotify named commands are reported without stopping the pipe",
    "[spotify][pipe][windows-integration]"
) {
    auto pipes = connect_pipe_pair();
    command_registry::Registry registry;
    recorded_actions actions;
    ac::pipes::CommandDispatcher dispatcher;
    bool protocol_failed = false;
    register_spotify_pipe_commands(
        dispatcher, pipes.server, registry, actions.pipe_actions(), protocol_failed
    );

    send_command(pipes.client, ac::protocol::component::Request::invoke);
    REQUIRE(ac::pipes::send_string(pipes.client, "missing_spotify_command"));
    send_command(pipes.client, ac::protocol::component::Request::shutdown);

    REQUIRE(dispatcher.process(pipes.server));
    CHECK_FALSE(protocol_failed);
    CHECK(actions.unknown_name == "missing_spotify_command");
    CHECK(actions.invoked == std::vector<std::string> {"shutdown"});
}

TEST_CASE(
    "A missing Spotify named-command payload marks the protocol failed",
    "[spotify][pipe][windows-integration]"
) {
    auto pipes = connect_pipe_pair();
    command_registry::Registry registry;
    recorded_actions actions;
    ac::pipes::CommandDispatcher dispatcher;
    bool protocol_failed = false;
    register_spotify_pipe_commands(
        dispatcher, pipes.server, registry, actions.pipe_actions(), protocol_failed
    );

    send_command(pipes.client, ac::protocol::component::Request::invoke);
    pipes.client.reset();

    REQUIRE(dispatcher.process(pipes.server));
    CHECK(protocol_failed);
    CHECK(dispatcher.stop_requested());
    CHECK(actions.invoked.empty());
}
