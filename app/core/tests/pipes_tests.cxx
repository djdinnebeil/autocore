#include "catch_amalgamated.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>
#include <Windows.h>

import auto_core.pipes;

namespace {

    struct ConnectedPipes {
        ac::pipes::Pipe server;
        ac::pipes::Pipe client;
    };

    std::wstring unique_pipe_name() {
        static std::atomic_uint counter {};

        return std::format(
            L"auto_core_tests_{}_{}",
            GetCurrentProcessId(),
            counter.fetch_add(1, std::memory_order_relaxed)
        );
    }

    ConnectedPipes connect_pipe_pair() {
        const std::wstring name = unique_pipe_name();
        auto server = ac::pipes::create_pipe_server(name);
        if (!server) {
            throw std::system_error(
                static_cast<int>(server.error().system_error),
                std::system_category(),
                "Could not create test pipe"
            );
        }

        auto client = ac::pipes::connect_to_pipe_server(name);
        if (!client) {
            throw std::system_error(
                static_cast<int>(client.error().system_error),
                std::system_category(),
                "Could not connect test pipe"
            );
        }

        return {std::move(*server), std::move(*client)};
    }

} // namespace

TEST_CASE("Pipe manages native handle ownership", "[pipes][windows-integration]") {
    ac::pipes::Pipe empty;
    CHECK_FALSE(empty.valid());
    CHECK_FALSE(empty.cancel());

    HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    REQUIRE(event != nullptr);

    ac::pipes::Pipe owner {event};
    REQUIRE(owner.valid());
    CHECK(owner.native_handle() == event);

    ac::pipes::Pipe moved {std::move(owner)};
    CHECK_FALSE(owner.valid());
    CHECK(moved.native_handle() == event);

    CHECK(moved.release() == event);
    CHECK_FALSE(moved.valid());
    REQUIRE(CloseHandle(event) != FALSE);

    event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    REQUIRE(event != nullptr);
    owner.reset(event);
    owner.reset();
    CHECK_FALSE(owner.valid());

    DWORD flags {};
    CHECK_FALSE(GetHandleInformation(event, &flags));
    CHECK(GetLastError() == ERROR_INVALID_HANDLE);
}

TEST_CASE(
    "Named pipes transfer strings in both directions",
    "[pipes][windows-integration]"
) {
    auto pipes = connect_pipe_pair();

    REQUIRE(ac::pipes::send_string(pipes.client, "client to server"));
    const auto from_client = ac::pipes::read_string(pipes.server);
    REQUIRE(from_client);
    CHECK(*from_client == "client to server");

    const std::string binary_message {'a', '\0', 'b'};
    REQUIRE(ac::pipes::send_string(pipes.server, binary_message));
    const auto from_server = ac::pipes::read_string(pipes.client);
    REQUIRE(from_server);
    CHECK(*from_server == binary_message);

    REQUIRE(ac::pipes::send_string(pipes.client, ""));
    const auto empty = ac::pipes::read_string(pipes.server);
    REQUIRE(empty);
    CHECK(empty->empty());
}

TEST_CASE(
    "String protocol rejects messages larger than one MiB",
    "[pipes][windows-integration]"
) {
    auto pipes = connect_pipe_pair();
    const std::string oversized(1024 * 1024 + 1, 'x');

    const auto result = ac::pipes::send_string(pipes.client, oversized);
    REQUIRE_FALSE(result);
    CHECK(result.error().system_error == ERROR_BAD_LENGTH);

    const std::uint32_t declared_size = 1024 * 1024 + 1;
    DWORD bytes_written {};
    REQUIRE(WriteFile(
        pipes.client.native_handle(),
        &declared_size,
        sizeof(declared_size),
        &bytes_written,
        nullptr
    ) != FALSE);
    REQUIRE(bytes_written == sizeof(declared_size));

    const auto received = ac::pipes::read_string(pipes.server);
    REQUIRE_FALSE(received);
    CHECK(received.error().system_error == ERROR_BAD_LENGTH);
}

TEST_CASE(
    "Pipe operations report invalid handles",
    "[pipes][windows-integration]"
) {
    ac::pipes::Pipe pipe;

    const auto command = ac::pipes::send_pipe_command(pipe, 1);
    REQUIRE_FALSE(command);
    CHECK(command.error().system_error == ERROR_INVALID_HANDLE);

    const auto sent = ac::pipes::send_string(pipe, "message");
    REQUIRE_FALSE(sent);
    CHECK(sent.error().system_error == ERROR_INVALID_HANDLE);

    const auto received = ac::pipes::read_string(pipe);
    REQUIRE_FALSE(received);
    CHECK(received.error().system_error == ERROR_INVALID_HANDLE);
}

TEST_CASE(
    "Command dispatcher invokes registered commands and stops",
    "[pipes][windows-integration]"
) {
    auto pipes = connect_pipe_pair();
    ac::pipes::CommandDispatcher dispatcher;
    int invocation_count = 0;

    dispatcher.set_command(42, [&]() {
        ++invocation_count;
        dispatcher.request_stop();
    });

    REQUIRE(ac::pipes::send_pipe_command(pipes.client, 42));
    const auto result = dispatcher.process(pipes.server);

    CHECK(result.has_value());
    CHECK(invocation_count == 1);
    CHECK(dispatcher.stop_requested());
}

TEST_CASE(
    "Command dispatcher rejects unregistered commands",
    "[pipes][windows-integration]"
) {
    auto pipes = connect_pipe_pair();
    ac::pipes::CommandDispatcher dispatcher;

    REQUIRE(ac::pipes::send_pipe_command(pipes.client, 404));
    const auto result = dispatcher.process(pipes.server);

    REQUIRE_FALSE(result);
    CHECK(result.error().system_error == ERROR_INVALID_DATA);
}
