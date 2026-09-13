module auto_core.main.components.writer;

import std;
import auto_core.main.application;
import auto_core.core.paths;
import auto_core.core.pipes;

import <Windows.h>;

namespace {

ac::pipes::Pipe writer_pipe;
std::mutex writer_pipe_mutex;

void invoke_writer(std::string_view expression) {
    const std::scoped_lock lock {writer_pipe_mutex};
    if (!writer_pipe.valid()) {
        auto_core.logg_and_print("writer_ac.exe is not connected.");
        return;
    }
    if (const auto request = ac::pipes::send_pipe_command(
            writer_pipe,
            ac::protocol::writer::to_wire(
                ac::protocol::writer::Request::invoke
            )
        ); !request) {
        auto_core.logg_and_print(
            "Failed to send writer command header. Error: {}",
            request.error().system_error
        );
        return;
    }
    if (const auto payload = ac::pipes::send_string(
            writer_pipe, expression
        ); !payload) {
        auto_core.logg_and_print(
            "Failed to send writer command. Error: {}",
            payload.error().system_error
        );
    }
}

} // namespace

void create_writer_pipe() {
    auto result = ac::pipes::create_pipe_server(
        std::wstring {ac::protocol::writer::pipe_name}
    );
    if (!result) {
        auto_core.logg_and_print(
            "Failed to create writer pipe. Error: {}",
            result.error().system_error
        );
        return;
    }
    const std::scoped_lock lock {writer_pipe_mutex};
    writer_pipe = std::move(*result);
}

void start_writer_component() {
    if (!ac::main::create_process(
            ac::paths::executable_directory() / "writer_ac.exe"
        )) {
        auto_core.logg_and_print("Unable to start writer_ac.exe.");
    }
}

bool wait_for_writer_ready() {
    using ReadyResult = ac::pipes::Result<std::string>;
    std::promise<ReadyResult> result_promise;
    std::future<ReadyResult> result = result_promise.get_future();

    std::jthread reader([&result_promise] {
        const std::scoped_lock lock {writer_pipe_mutex};
        constexpr auto retry_interval = std::chrono::milliseconds {10};
        const auto deadline = std::chrono::steady_clock::now() +
            std::chrono::seconds {5};

        while (true) {
            auto message = ac::pipes::read_string(writer_pipe);
            if (message ||
                (message.error().system_error != ERROR_PIPE_LISTENING &&
                 message.error().system_error != ERROR_PIPE_NOT_CONNECTED) ||
                std::chrono::steady_clock::now() >= deadline) {
                result_promise.set_value(std::move(message));
                return;
            }
            std::this_thread::sleep_for(retry_interval);
        }
    });

    if (result.wait_for(std::chrono::seconds {5}) ==
        std::future_status::ready) {
        const ReadyResult response = result.get();
        if (!response) {
            auto_core.logg_and_print(
                "Failed to read writer readiness response. Error: {}",
                response.error().system_error
            );
            return false;
        }
        if (*response != ac::protocol::writer::ready_message) {
            auto_core.logg_and_print(
                "Unexpected writer readiness response: {}", *response
            );
            return false;
        }
        return true;
    }

    writer_pipe.cancel();
    reader.join();
    auto_core.logg_and_print(
        "Timed out waiting for writer component readiness"
    );
    return false;
}

void send_writer_end_signal() {
    const std::scoped_lock lock {writer_pipe_mutex};
    if (!writer_pipe.valid()) {
        return;
    }
    if (const auto result = ac::pipes::send_pipe_command(
            writer_pipe,
            ac::protocol::writer::to_wire(
                ac::protocol::writer::Request::shutdown
            )
        ); !result) {
        auto_core.logg_and_print(
            "Failed to stop writer. Error: {}",
            result.error().system_error
        );
    }
    writer_pipe.reset();
}

void writer_component::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    std::unordered_set<std::string> names;
    std::ifstream input(
        ac::paths::keymap_components_directory() /
        ac::protocol::writer::manifest_filename
    );
    const bool manifest_available = input.is_open();
    std::string name;
    while (std::getline(input, name)) {
        if (!name.empty() && name.back() == '\r') {
            name.pop_back();
        }
        if (name.starts_with("\xEF\xBB\xBF")) {
            name.erase(0, 3);
        }
        if (!name.empty()) {
            names.emplace(std::move(name));
        }
    }
    if (!manifest_available) {
        for (const auto command : ac::protocol::writer::commands) {
            names.emplace(command.name);
        }
    }
    for (const std::string& value : names) {
        registry.add(value, [value] { invoke_writer(value); });
    }
}
