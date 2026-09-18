/**
 * \file main.cxx
 * \brief Generic-host smoke child. Speaks ac.component.v1 and prints one
 * keymap command so a new component can be added without rebuilding Main.
 */
import std;
import auto_core.core.clock;
import auto_core.core.component;
import auto_core.core.pipes;
import command_registry;
import component_protocol;

import <Windows.h>;

namespace {

constexpr std::string_view component_name = "simple_test";
constexpr std::string_view executable_name = "simple_test_ac.exe";

ac::Component& simple_test_component() {
    static ac::Component component {component_name};
    return component;
}

command_registry::Registry create_simple_test_command_registry() {
    command_registry::Registry registry;
    registry.add("print_simple_test", [] {
        simple_test_component().log_and_print(
            "this is print_simple_test() from within simple_test_ac.exe"
        );
    });
    return registry;
}

int write_manifest(
    const command_registry::Registry& registry,
    const std::filesystem::path& destination
) {
    std::filesystem::path temporary = destination;
    temporary += ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        return 1;
    }
    for (const std::string& value : registry.autocomplete_values()) {
        output << value << '\n';
    }
    output.close();
    if (!output) {
        return 1;
    }
    return MoveFileExW(
        temporary.c_str(),
        destination.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
    ) ? 0 : 1;
}

[[nodiscard]]
std::string_view resolved_name(const std::string_view expression) noexcept {
    const auto opening = expression.find('(');
    if (opening == std::string_view::npos) {
        return expression;
    }
    return expression.substr(0, opening);
}

} // namespace

int main(int argument_count, char* arguments[]) {
    auto registry = create_simple_test_command_registry();

    if (argument_count == 3 &&
        std::string_view {arguments[1]} ==
            "--generate-keymap-command-registry") {
        return write_manifest(registry, arguments[2]);
    }

    auto& component = simple_test_component();
    component.connect_to_logger();

    const auto pipe_name = ac::protocol::component::pipe_name(component_name);
    component.log_and_print("simple_test_ac.exe starting");
    component.log_and_print("component: {}", component_name);
    component.log_and_print("executable: {}", executable_name);
    component.log_and_print("process ID: {}", GetCurrentProcessId());
    component.log_and_print("pipe: {}", pipe_name);

    auto connection = ac::pipes::connect_to_pipe_server(pipe_name);
    if (!connection) {
        component.log_and_print("pipe connection: failed");
        component.log_and_print(
            "Failed to connect to simple_test pipe. Error: {}",
            connection.error().system_error
        );
        component.log_and_print("shutdown: {}", ac::clock::get_datetime());
        return 1;
    }

    component.log_and_print("pipe connection: connected");
    component.log_and_print("startup: {}", ac::clock::get_datetime());
    component.log_and_print("waiting for commands");

    ac::pipes::Pipe pipe = std::move(*connection);
    ac::pipes::CommandDispatcher dispatcher;
    bool protocol_failed = false;
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::invoke
        ),
        [&pipe, &registry, &dispatcher, &protocol_failed, &component] {
            const auto expression = ac::pipes::read_string(pipe);
            if (!expression) {
                component.log_and_print(
                    "Failed to read simple_test command. Error: {}",
                    expression.error().system_error
                );
                protocol_failed = true;
                dispatcher.request_stop();
                return;
            }

            component.log_and_print("received command: {}", *expression);
            component.log_and_print(
                "resolved function: {}",
                resolved_name(*expression)
            );

            auto action = registry.resolve(*expression);
            if (!action) {
                component.log_and_print(
                    "Unknown simple_test command: {}",
                    *expression
                );
                component.log_and_print("command failed");
                return;
            }

            component.log_and_print("executing: {}", *expression);
            action();
            component.log_and_print("command completed successfully");
        }
    );
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::shutdown
        ),
        [&dispatcher, &component] {
            component.log_and_print("shutdown signal received");
            dispatcher.request_stop();
        }
    );

    if (const auto hello = ac::pipes::send_string(
            pipe, ac::protocol::component::make_hello(
                registry.autocomplete_values()
            )
        ); !hello) {
        component.log_and_print(
            "Failed to send simple_test hello. Error: {}",
            hello.error().system_error
        );
        component.log_and_print("shutdown: {}", ac::clock::get_datetime());
        return 1;
    }

    if (const auto result = dispatcher.process(pipe); !result) {
        component.log_and_print(
            "simple_test pipe failed. Error: {}",
            result.error().system_error
        );
        component.log_and_print("shutdown: {}", ac::clock::get_datetime());
        return 1;
    }
    if (protocol_failed) {
        component.log_and_print("shutdown: {}", ac::clock::get_datetime());
        return 1;
    }

    component.log_and_print("program terminated");
    return 0;
}
