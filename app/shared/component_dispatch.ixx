/**
 * \file component_dispatch.ixx
 * \brief First-argument operation selection for a component executable.
 *
 * The operation is only argv[1]. The returned argument vector keeps argv[0]
 * and drops that operation token so the existing parser sees the tail it
 * sees today. No arguments is not an operation.
 */
export module component_dispatch;

import std;

export namespace ac::component_dispatch {

    enum class Operation {
        none,
        run,
        settings,
        config,
        invalid
    };

    struct Command {
        Operation operation {Operation::none};
        std::vector<char*> arguments;
    };

    [[nodiscard]]
    inline Command parse(const int argc, char* argv[]) {
        Command command;
        if (argc <= 1 || argv == nullptr || argv[1] == nullptr) {
            command.operation = Operation::none;
            return command;
        }

        const std::string_view token {argv[1]};
        if (token == "--run") {
            command.operation = Operation::run;
        }
        else if (token == "--settings") {
            command.operation = Operation::settings;
        }
        else if (token == "--config") {
            command.operation = Operation::config;
        }
        else {
            command.operation = Operation::invalid;
            return command;
        }

        command.arguments.push_back(argv[0]);
        for (int index = 2; index < argc; ++index) {
            command.arguments.push_back(argv[index]);
        }
        return command;
    }

} // namespace ac::component_dispatch
