module auto_core.main.components.journal;

import std;
import auto_core.main.application;
import auto_core.core.paths;
import auto_core.core.pipes;

import <Windows.h>;

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

command_registry::Factory make_remote_factory(std::string factory_name) {
    return [factory_name = std::move(factory_name)](std::string_view arguments) {
        const std::string expression =
            std::format("{}({})", factory_name, arguments);
        return command_registry::Action {[expression] { invoke_journal(expression); }};
    };
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
    ac::main::create_process(
        ac::paths::executable_directory() / "journal_ac.exe"
    );
}

bool wait_for_journal_ready() {
    using ReadyResult = ac::pipes::Result<std::string>;
    std::promise<ReadyResult> result_promise;
    std::future<ReadyResult> result = result_promise.get_future();

    std::jthread reader([&result_promise] {
        const std::scoped_lock lock {journal_pipe_mutex};
        constexpr auto retry_interval = std::chrono::milliseconds {10};
        const auto deadline = std::chrono::steady_clock::now() +
            std::chrono::seconds {5};

        while (true) {
            auto message = ac::pipes::read_string(journal_pipe);
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
                "Failed to read journal readiness response. Error: {}",
                response.error().system_error
            );
            return false;
        }
        if (*response != ac::protocol::journal::ready_message) {
            auto_core.logg_and_print(
                "Unexpected journal readiness response: {}",
                *response
            );
            return false;
        }
        return true;
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

std::function<void()> journal_print_choice_command(std::string name, bool include_zero) {
    const std::string expression = std::format(
        "make_print_choice(\"{}\", {})", name, include_zero ? "true" : "false"
    );
    return [expression] { invoke_journal(expression); };
}

std::function<void()> journal_print_choice_command(
    std::string name,
    int lower_bound,
    int count
) {
    const std::string expression = count == 1
        ? std::format("make_print_choice(\"{}\", {})", name, lower_bound)
        : std::format(
            "make_print_choice(\"{}\", {}, {})", name, lower_bound, count
        );
    return [expression] { invoke_journal(expression); };
}

std::function<void()> journal_print_and_insert_command(std::string text) {
    const std::string expression = std::format(
        "print_and_insert_into_journal(\"{}\")", text
    );
    return [expression] { invoke_journal(expression); };
}

void journal::runtime_commands::register_with(command_registry::Registry& registry) {
    std::unordered_set<std::string> names;
    const auto manifest = ac::paths::keymap_components_directory() /
        ac::protocol::journal::manifest_filename;
    std::ifstream input(manifest);
    const bool manifest_available = input.is_open();
    std::string value;
    bool print_choice_factory_added = false;
    bool insert_factory_added = false;

    while (std::getline(input, value)) {
        if (!value.empty() && value.back() == '\r') value.pop_back();
        if (value.empty()) continue;
        const std::size_t opening = value.find('(');
        if (opening == std::string::npos) {
            for (const auto command : ::journal_commands) {
                if (command.name == value) {
                    names.insert(value);
                    break;
                }
            }
        }
        else if (value.substr(0, opening) == "make_print_choice") {
            registry.add_factory(
                "make_print_choice",
                make_remote_factory("make_print_choice"),
                value
            );
            print_choice_factory_added = true;
        }
        else if (value.substr(0, opening) == "print_and_insert_into_journal") {
            registry.add_factory(
                "print_and_insert_into_journal",
                make_remote_factory("print_and_insert_into_journal"),
                value
            );
            insert_factory_added = true;
        }
    }

    if (!manifest_available) {
        for (const auto command : ::journal_commands) names.emplace(command.name);
    }
    if (!print_choice_factory_added) {
        registry.add_factory(
            "make_print_choice",
            make_remote_factory("make_print_choice"),
            R"(make_print_choice("", false))"
        );
    }
    if (!insert_factory_added) {
        registry.add_factory(
            "print_and_insert_into_journal",
            make_remote_factory("print_and_insert_into_journal"),
            R"(print_and_insert_into_journal(""))"
        );
    }
    for (const std::string& name : names) {
        registry.add(name, [name] { invoke_journal(name); });
    }
    registry.add("launch_journal_config", [] {
        const auto executable =
            ac::paths::executable_directory() / "journal_config.exe";
        if (!ac::main::create_process_and_focus(
                executable, {}, CREATE_NEW_CONSOLE
            )) {
            auto_core.logg_and_print(
                "Unable to start journal_config.exe."
            );
        }
    });
}
