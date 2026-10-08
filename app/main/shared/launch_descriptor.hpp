/**
 * \file launch_descriptor.hpp
 * \brief Parser for a component's embedded launch descriptor.
 *
 * The descriptor is RCDATA named `AC_LAUNCH_DESCRIPTOR` inside `{name}_ac.exe`.
 * Hosted components do not embed it. Main does not recognize components by name.
 *
 * Lines are `key=value` or a command name. Blank lines and `#` comments
 * are ignored. Defaults match a command-argument launch that does not wait:
 * `console=default`, `context=none`, `wait=none`, `argv=command`.
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ac::main::launch_descriptor {

    inline constexpr wchar_t embedded_resource_name[] = L"AC_LAUNCH_DESCRIPTOR";

    struct Descriptor {
        enum class Console { inherit, fresh };
        enum class Context { none, foreground };
        enum class Wait { none, infinite };
        enum class Arguments { command, none };

        Console console {Console::inherit};
        Context context {Context::none};
        Wait wait {Wait::none};
        Arguments arguments {Arguments::command};
        std::vector<std::string> commands;
    };

    struct ParseResult {
        bool ok {false};
        std::string error;
        Descriptor descriptor;
    };

    [[nodiscard]]
    inline std::string_view trim_line(const std::string_view value) noexcept {
        const auto first = value.find_first_not_of(" \t\r");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t\r");
        return value.substr(first, last - first + 1);
    }

    [[nodiscard]]
    inline ParseResult parse(std::string_view text) {
        if (text.starts_with("\xEF\xBB\xBF")) {
            text.remove_prefix(3);
        }

        ParseResult result;
        result.ok = true;
        bool console_set = false;
        bool context_set = false;
        bool wait_set = false;
        bool arguments_set = false;

        const auto fail = [&](std::string message) {
            result.ok = false;
            result.error = std::move(message);
            result.descriptor = {};
        };

        std::size_t line_start = 0;
        while (result.ok && line_start <= text.size()) {
            const auto line_end = text.find('\n', line_start);
            const auto length = line_end == std::string_view::npos
                ? text.size() - line_start
                : line_end - line_start;
            const auto line = trim_line(text.substr(line_start, length));
            line_start = line_end == std::string_view::npos
                ? text.size() + 1
                : line_end + 1;

            if (line.empty() || line.starts_with('#')) {
                continue;
            }

            const auto assignment = line.find('=');
            if (assignment == std::string_view::npos) {
                const std::string command {line};
                for (const auto& existing : result.descriptor.commands) {
                    if (existing == command) {
                        fail("duplicate command '" + command + "'");
                        break;
                    }
                }
                if (result.ok) {
                    result.descriptor.commands.push_back(command);
                }
                continue;
            }

            const auto key = trim_line(line.substr(0, assignment));
            const auto value = trim_line(line.substr(assignment + 1));
            if (key == "console") {
                if (console_set) {
                    fail("duplicate console");
                }
                else if (value == "default") {
                    result.descriptor.console = Descriptor::Console::inherit;
                    console_set = true;
                }
                else if (value == "new") {
                    result.descriptor.console = Descriptor::Console::fresh;
                    console_set = true;
                }
                else {
                    fail("invalid console value");
                }
            }
            else if (key == "context") {
                if (context_set) {
                    fail("duplicate context");
                }
                else if (value == "none") {
                    result.descriptor.context = Descriptor::Context::none;
                    context_set = true;
                }
                else if (value == "foreground") {
                    result.descriptor.context = Descriptor::Context::foreground;
                    context_set = true;
                }
                else {
                    fail("invalid context value");
                }
            }
            else if (key == "wait") {
                if (wait_set) {
                    fail("duplicate wait");
                }
                else if (value == "none") {
                    result.descriptor.wait = Descriptor::Wait::none;
                    wait_set = true;
                }
                else if (value == "infinite") {
                    result.descriptor.wait = Descriptor::Wait::infinite;
                    wait_set = true;
                }
                else {
                    fail("invalid wait value");
                }
            }
            else if (key == "argv") {
                if (arguments_set) {
                    fail("duplicate argv");
                }
                else if (value == "command") {
                    result.descriptor.arguments = Descriptor::Arguments::command;
                    arguments_set = true;
                }
                else if (value == "none") {
                    result.descriptor.arguments = Descriptor::Arguments::none;
                    arguments_set = true;
                }
                else {
                    fail("invalid argv value");
                }
            }
            else {
                fail("unknown key '" + std::string {key} + "'");
            }
        }

        if (result.ok && result.descriptor.commands.empty()) {
            fail("launch descriptor has no commands");
        }
        return result;
    }

} // namespace ac::main::launch_descriptor
