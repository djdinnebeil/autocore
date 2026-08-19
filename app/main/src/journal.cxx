module journal;

import std;
import ac_main;
import auto_core.paths;
import auto_core.pipes;
import journey;
import taskbar;

namespace {
ac::pipes::Pipe journal_pipe;
std::mutex journal_pipe_mutex;

void invoke_journal(std::string_view expression) {
    const std::scoped_lock lock {journal_pipe_mutex};
    if (const auto request = ac::pipes::send_pipe_command(
            journal_pipe,
            ac::protocol::journal::to_wire(ac::protocol::journal::Request::invoke)
        ); !request) {
        auto_core.logg_and_print("Failed to send journal command header. Error: {}", request.error().system_error);
        return;
    }
    if (const auto payload = ac::pipes::send_string(journal_pipe, expression); !payload) {
        auto_core.logg_and_print("Failed to send journal command. Error: {}", payload.error().system_error);
    }
}

command_registry::Action make_remote_factory_action(std::string_view arguments) {
    const std::string expression = std::format("make_print_choice({})", arguments);
    return [expression] { invoke_journal(expression); };
}
} // namespace

void create_journal_pipe() {
    auto result = ac::pipes::create_pipe_server(std::wstring {ac::protocol::journal::pipe_name});
    if (!result) {
        auto_core.logg_and_print("Failed to create journal pipe. Error: {}", result.error().system_error);
        return;
    }
    journal_pipe = std::move(*result);
}

void start_journal_component() {
    std::wstring arguments;
    if (const auto position = taskbar_position("auto_core")) {
        arguments = std::format(L"--taskbar-position {}", *position);
    }
    else {
        auto_core.logg_and_print(
            "Auto Core taskbar position is not configured for journal prompts."
        );
    }
    ac::main::create_process(
        ac::paths::executable_directory() / "journal.exe", arguments
    );
}

bool wait_for_journal_ready() {
    std::promise<bool> result_promise;
    std::future<bool> result = result_promise.get_future();

    std::jthread reader([&result_promise] {
        const std::scoped_lock lock {journal_pipe_mutex};
        const auto message = ac::pipes::read_string(journal_pipe);
        result_promise.set_value(
            message && *message == ac::protocol::journal::ready_message
        );
    });

    if (result.wait_for(std::chrono::seconds {5}) ==
        std::future_status::ready) {
        const bool ready = result.get();
        if (!ready) {
            auto_core.logg_and_print(
                "Journal component returned an invalid readiness response"
            );
        }
        return ready;
    }

    journal_pipe.cancel();
    reader.join();
    auto_core.logg_and_print(
        "Timed out waiting for journal component readiness"
    );
    return false;
}

void send_journal_end_signal() {
    const std::scoped_lock lock {journal_pipe_mutex};
    if (const auto result = ac::pipes::send_pipe_command(
            journal_pipe,
            ac::protocol::journal::to_wire(ac::protocol::journal::Request::shutdown)
        ); !result) {
        auto_core.logg_and_print("Failed to stop journal. Error: {}", result.error().system_error);
    }
}

std::function<void()> journal_command(ac::protocol::journal::Command command) {
    return [name = std::string {command.name}] { invoke_journal(name); };
}

std::function<void()> journal_print_choice_command(std::string name, bool include_zero) {
    const std::string expression = std::format(
        "make_print_choice(\"{}\", {})", name, include_zero ? "true" : "false"
    );
    return [expression] { invoke_journal(expression); };
}

void journal::runtime_commands::register_with(command_registry::Registry& registry) {
    std::unordered_set<std::string> names;
    const auto manifest = ac::paths::executable_directory() /
        ac::protocol::journal::manifest_filename;
    std::ifstream input(manifest);
    const bool manifest_available = input.is_open();
    std::string value;
    bool factory_added = false;

    while (std::getline(input, value)) {
        if (!value.empty() && value.back() == '\r') value.pop_back();
        if (value.empty()) continue;
        const std::size_t opening = value.find('(');
        if (opening == std::string::npos) {
            names.insert(value);
        }
        else if (value.substr(0, opening) == "make_print_choice") {
            registry.add_factory("make_print_choice", make_remote_factory_action, value);
            factory_added = true;
        }
    }

    if (!manifest_available) {
        for (const auto command : ::journal_commands) names.emplace(command.name);
    }
    if (!factory_added) {
        registry.add_factory("make_print_choice", make_remote_factory_action,
            R"(make_print_choice("", false))");
    }
    for (const std::string& name : names) {
        registry.add(name, [name] { invoke_journal(name); });
    }
}
