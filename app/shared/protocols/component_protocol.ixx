/**
 * \file component_protocol.ixx
 * \brief Shared Auto Core generic component control protocol (v1).
 *
 * Sequence: Main creates `ac_{name}_pipe`, starts `{name}_ac.exe`, and keeps
 * the process handle for the session. The child connects and sends one
 * length-prefixed hello. After hello, Main sends `invoke` plus one expression
 * string, or `shutdown` with no payload. Do not reuse or renumber request IDs.
 *
 * A catalog line is a command name, or a factory expression `name(...)`.
 * A command that declares a Taskbar application key is `name @key`. The key
 * is not part of the command name. An optional description follows that
 * complete identity: ` - description`. The description may contain further
 * hyphens. It is not part of the name or the key.
 */
export module component_protocol;

import std;

export namespace ac::protocol::component {

    /** First line of the hello/catalog payload. */
    inline constexpr std::string_view protocol_id = "ac.component.v2";
    inline constexpr std::string_view catalog_description_separator = " - ";

    /**
     * \brief Control requests sent by Main after hello.
     *
     * Frozen. New operations, if ever needed, take the next unused value.
     */
    enum class Request : std::int32_t {
        invoke = 0,
        shutdown = 1,
    };

    enum class TerminationPolicy {
        graceful,
        force_allowed,
    };

    [[nodiscard]]
    constexpr std::string_view to_string(
        const TerminationPolicy policy
    ) noexcept {
        switch (policy) {
        case TerminationPolicy::force_allowed:
            return "force_allowed";
        case TerminationPolicy::graceful:
        default:
            return "graceful";
        }
    }

    [[nodiscard]] constexpr std::int32_t to_wire(Request request) noexcept {
        return static_cast<std::int32_t>(request);
    }

    enum class HelloError {
        empty,
        version_mismatch,
    };

    struct CatalogEntry {
        std::string name;
        bool accepts_arguments {};
        std::string autocomplete;
        std::string description;
        /** Declared Taskbar application key. Empty when the command has none. */
        std::string application;
    };

    struct Hello {
        TerminationPolicy termination_policy {TerminationPolicy::graceful};
        std::vector<CatalogEntry> commands;
        std::vector<std::string> skipped_lines;
    };

    [[nodiscard]]
    inline std::string executable_name(const std::string_view component_name) {
        std::string name {component_name};
        name += "_ac.exe";
        return name;
    }

    [[nodiscard]]
    inline std::wstring pipe_name(const std::string_view component_name) {
        std::wstring name {L"ac_"};
        name.append(component_name.begin(), component_name.end());
        name += L"_pipe";
        return name;
    }

    [[nodiscard]]
    inline std::string make_hello(
        const std::vector<std::string>& catalog_lines,
        const TerminationPolicy termination_policy =
            TerminationPolicy::graceful
    ) {
        std::string payload;
        payload += protocol_id;
        payload += '\n';
        payload += "termination_policy = ";
        payload += to_string(termination_policy);
        payload += '\n';
        for (const auto& line : catalog_lines) {
            payload += line;
            payload += '\n';
        }
        return payload;
    }

    [[nodiscard]]
    inline std::string_view trim_hello_line(const std::string_view value) noexcept {
        const auto first = value.find_first_not_of(" \t\r");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t\r");
        return value.substr(first, last - first + 1);
    }

    [[nodiscard]]
    inline bool is_command_name(const std::string_view name) noexcept {
        if (name.empty()) {
            return false;
        }
        const auto first = static_cast<unsigned char>(name.front());
        if (first != '_' && !std::isalpha(first)) {
            return false;
        }
        return std::ranges::all_of(name, [](const char character) {
            const auto value = static_cast<unsigned char>(character);
            return value == '_' || std::isalnum(value);
        });
    }

    [[nodiscard]]
    inline bool is_application_key(const std::string_view key) noexcept {
        if (key.empty()) {
            return false;
        }
        const auto first = static_cast<unsigned char>(key.front());
        if (!std::isalpha(first)) {
            return false;
        }
        return std::ranges::all_of(key, [](const char character) {
            const auto value = static_cast<unsigned char>(character);
            return std::isalnum(value) || character == '_';
        });
    }

    /**
     * \brief Splits `name` or `name @key`.
     *
     * The command name and the application key stay separate. A missing key
     * is empty. An `@` without a valid key is rejected.
     */
    [[nodiscard]]
    inline std::optional<std::pair<std::string, std::string>>
    parse_command_identity(const std::string_view identity) {
        const auto marker = identity.find(" @");
        const std::string_view name = marker == std::string_view::npos
            ? identity
            : trim_hello_line(identity.substr(0, marker));
        if (!is_command_name(name)) {
            return std::nullopt;
        }
        if (marker == std::string_view::npos) {
            return std::pair {std::string {name}, std::string {}};
        }
        const auto key = trim_hello_line(identity.substr(marker + 2));
        if (!is_application_key(key)) {
            return std::nullopt;
        }
        return std::pair {std::string {name}, std::string {key}};
    }

    [[nodiscard]]
    inline std::optional<CatalogEntry> parse_catalog_line(
        const std::string_view line
    ) {
        const auto trimmed = trim_hello_line(line);
        if (trimmed.empty()) {
            return std::nullopt;
        }

        const auto opening = trimmed.find('(');
        if (opening != std::string_view::npos) {
            const auto name = trim_hello_line(trimmed.substr(0, opening));
            if (!is_command_name(name)) {
                return std::nullopt;
            }

            int depth = 0;
            std::size_t close = std::string_view::npos;
            for (std::size_t index = opening; index < trimmed.size(); ++index) {
                if (trimmed[index] == '(') {
                    ++depth;
                }
                else if (trimmed[index] == ')') {
                    --depth;
                    if (depth == 0) {
                        close = index;
                        break;
                    }
                }
            }
            if (close == std::string_view::npos) {
                return std::nullopt;
            }

            const auto identity = trimmed.substr(0, close + 1);
            const auto rest = trimmed.substr(close + 1);
            std::string description;
            if (!rest.empty()) {
                if (!rest.starts_with(catalog_description_separator)) {
                    return std::nullopt;
                }
                description = std::string {trim_hello_line(
                    rest.substr(catalog_description_separator.size())
                )};
            }

            return CatalogEntry {
                std::string {name},
                true,
                std::string {identity},
                std::move(description)
            };
        }

        const auto separator = trimmed.find(catalog_description_separator);
        const auto identity = separator == std::string_view::npos
            ? trimmed
            : trim_hello_line(trimmed.substr(0, separator));
        const auto parsed = parse_command_identity(identity);
        if (!parsed) {
            return std::nullopt;
        }

        std::string description;
        if (separator != std::string_view::npos) {
            description = std::string {trim_hello_line(
                trimmed.substr(separator + catalog_description_separator.size())
            )};
        }

        return CatalogEntry {
            parsed->first,
            false,
            parsed->first,
            std::move(description),
            parsed->second
        };
    }

    [[nodiscard]]
    inline std::expected<Hello, HelloError> parse_hello(
        std::string_view payload
    ) {
        if (payload.starts_with("\xEF\xBB\xBF")) {
            payload.remove_prefix(3);
        }

        if (payload.empty()) {
            return std::unexpected(HelloError::empty);
        }

        Hello hello;
        bool first_line = true;
        bool termination_policy_seen = false;
        std::size_t line_start = 0;
        while (line_start <= payload.size()) {
            const auto line_end = payload.find('\n', line_start);
            const auto length = line_end == std::string_view::npos
                ? payload.size() - line_start
                : line_end - line_start;
            const auto line = trim_hello_line(payload.substr(line_start, length));

            if (first_line) {
                if (line != protocol_id) {
                    return std::unexpected(HelloError::version_mismatch);
                }
                first_line = false;
            }
            else if (!line.empty()) {
                const auto assignment = line.find('=');
                const auto key = assignment == std::string_view::npos
                    ? std::string_view {}
                    : trim_hello_line(line.substr(0, assignment));
                if (key == "termination_policy") {
                    const auto value = trim_hello_line(
                        line.substr(assignment + 1)
                    );
                    if (termination_policy_seen) {
                        hello.termination_policy = TerminationPolicy::graceful;
                        hello.skipped_lines.emplace_back(line);
                    }
                    else if (value == "force_allowed") {
                        hello.termination_policy =
                            TerminationPolicy::force_allowed;
                    }
                    else if (value == "graceful") {
                        hello.termination_policy =
                            TerminationPolicy::graceful;
                    }
                    else {
                        hello.skipped_lines.emplace_back(line);
                    }
                    termination_policy_seen = true;
                }
                else if (line.starts_with("termination_policy")) {
                    hello.skipped_lines.emplace_back(line);
                }
                else if (auto entry = parse_catalog_line(line)) {
                    hello.commands.push_back(std::move(*entry));
                }
                else {
                    hello.skipped_lines.emplace_back(line);
                }
            }

            if (line_end == std::string_view::npos) {
                break;
            }
            line_start = line_end + 1;
        }

        if (first_line) {
            return std::unexpected(HelloError::empty);
        }

        return hello;
    }

} // namespace ac::protocol::component
