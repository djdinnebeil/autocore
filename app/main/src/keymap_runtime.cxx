module auto_core.main.keymap.runtime;

import std;

import auto_core.core.config;
import auto_core.core.ini;
import auto_core.core.paths;
import auto_core.main.components;
import command_registry;
import auto_core.main.key_codes;
import auto_core.main.keymap;
import auto_core.main.application;
import auto_core.main.taskbar;
import auto_core.main.test_commands;

import <Windows.h>;

static command_registry::Registry create_runtime_command_registry() {
    command_registry::Registry registry;

    test_commands::runtime_commands::register_with(registry);
    main_component::runtime_commands::register_with(registry);
    dash::runtime_commands::register_with(registry);
    itunes_component::runtime_commands::register_with(registry);
    journal::runtime_commands::register_with(registry);
    slash_component::runtime_commands::register_with(registry);
    spotify_component::runtime_commands::register_with(registry);
    taskbar_component::runtime_commands::register_with(registry);
    writer_component::runtime_commands::register_with(registry);
    // Configuration-defined taskbar commands are registered last so a
    // manual INI file can never override a reserved Main command.
    taskbar_runtime_commands::register_with(registry);

    return registry;
}

static const command_registry::Registry& runtime_command_registry() {
    static const command_registry::Registry registry =
        create_runtime_command_registry();
    return registry;
}

static std::string_view trim(std::string_view value);
static std::vector<std::string> journal_alias_names();

template<typename ContentsFactory>
static bool write_new_file(
    const std::filesystem::path& path,
    ContentsFactory contents_factory
) {
    std::error_code ec;

    if (std::filesystem::exists(path, ec)) {
        return true;
    }

    if (ec) {
        auto_core.logg_and_logg(
            "Failed to inspect {}: {}",
            path.string(),
            ec.message()
        );
        return false;
    }

    const std::string contents = contents_factory();
    std::ofstream output(path, std::ios::binary);

    if (!output) {
        auto_core.logg_and_logg(
            "Failed to create {}",
            path.string()
        );
        return false;
    }

    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );
    output.close();

    if (!output) {
        auto_core.logg_and_logg(
            "Failed to write {}",
            path.string()
        );
        return false;
    }

    return true;
}

static std::string seed_keymap_contents() {
    std::string contents = "[keymap]\n";
    for (const auto& key : key_codes::keys) {
        if (key.name == "numpad_0") {
            contents +=
                "numpad_0 = {activate_function_key, deactivate_function_key}\n";
        }
        else if (key.name == "numpad_1") {
            contents +=
                "numpad_1 = {activate_auto_core, close_program}\n";
        }
        else {
            contents += std::format("{} = {{, }}\n", key.name);
        }
    }
    return contents;
}

static bool initialize_keymap_workspace() {
    std::error_code ec;
    std::filesystem::create_directories(
        ac::paths::keymap_directory(),
        ec
    );

    if (ec) {
        auto_core.logg_and_logg(
            "Failed to create keymap directory: {}",
            ec.message()
        );
        return false;
    }

    std::filesystem::create_directories(
        ac::paths::keymap_components_directory(),
        ec
    );

    if (ec) {
        auto_core.logg_and_logg(
            "Failed to create keymap components directory: {}",
            ec.message()
        );
        return false;
    }

    return write_new_file(
        ac::paths::keymap_file(),
        seed_keymap_contents
    );
}

struct KeymapSettings {
    bool trace_enabled = false;
    bool silence_nonset_warning = false;
};

static KeymapSettings keymap_settings() {
    const auto document =
        ac::ini::read(ac::paths::keymap_settings_file());
    if (!document) {
        return {};
    }

    return {
        document->find("keymap", "trace_enabled") == "true",
        document->find("keymap", "silence_nonset_warning") == "true"
    };
}

static std::string autocomplete_contents(
    const command_registry::Registry& registry
) {
    std::vector<std::string> values = registry.autocomplete_values();
    const auto aliases = journal_alias_names();
    values.insert(values.end(), aliases.begin(), aliases.end());
    std::ranges::sort(values);

    std::size_t required_size = values.size();

    for (const std::string& value : values) {
        required_size += value.size();
    }

    std::string contents;
    contents.reserve(required_size);

    for (const std::string& value : values) {
        contents += value;
        contents += '\n';
    }

    return contents;
}

static bool replace_text_file(
    const std::filesystem::path& destination,
    const std::string& desired,
    std::string_view failed_create,
    std::string_view failed_write,
    std::string_view failed_replace
) {
    {
        std::ifstream existing_file(destination, std::ios::binary);

        if (existing_file) {
            const std::string existing {
                std::istreambuf_iterator<char> {existing_file},
                std::istreambuf_iterator<char> {}
            };

            if (existing == desired) {
                return true;
            }
        }
    }

    std::filesystem::path temporary = destination;
    temporary += ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);

    if (!output) {
        auto_core.logg_and_logg("{}", failed_create);
        return false;
    }

    output.write(
        desired.data(),
        static_cast<std::streamsize>(desired.size())
    );
    output.close();

    if (!output) {
        auto_core.logg_and_logg("{}", failed_write);
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return false;
    }

    if (!MoveFileExW(
        temporary.c_str(),
        destination.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
    )) {
        const DWORD error = GetLastError();
        auto_core.logg_and_logg("{}: {}", failed_replace, error);
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return false;
    }

    return true;
}

static std::string journal_keymap_commands_contents() {
    std::string print_choice_factory {R"(make_print_choice("", false))"};
    std::string insert_factory {R"(print_and_insert_into_journal(""))"};

    const auto manifest = ac::paths::keymap_components_directory() /
        ac::protocol::journal::manifest_filename;
    std::ifstream input(manifest);
    std::string value;
    while (std::getline(input, value)) {
        if (!value.empty() && value.back() == '\r') value.pop_back();
        if (value.empty()) continue;
        const std::size_t opening = value.find('(');
        if (opening == std::string::npos) continue;
        const auto name = value.substr(0, opening);
        if (name == "make_print_choice") {
            print_choice_factory = value;
        }
        else if (name == "print_and_insert_into_journal") {
            insert_factory = value;
        }
    }

    std::vector<std::string> lines;
    lines.push_back(std::move(print_choice_factory));
    lines.push_back(std::move(insert_factory));
    for (const auto command : ::journal_commands) {
        lines.emplace_back(command.name);
    }
    for (const auto& name : journal_alias_names()) {
        lines.push_back(name);
    }
    std::ranges::sort(lines);

    std::string contents;
    for (const std::string& line : lines) {
        contents += line;
        contents += '\n';
    }
    return contents;
}

static bool refresh_journal_keymap_commands() {
    return replace_text_file(
        ac::paths::keymap_components_directory() /
            ac::protocol::journal::manifest_filename,
        journal_keymap_commands_contents(),
        "Failed to create temporary journal keymap command list",
        "Failed to write temporary journal keymap command list",
        "Failed to replace journal keymap command list"
    );
}

static bool refresh_keymap_commands(
    const command_registry::Registry& registry
) {
    return replace_text_file(
        ac::paths::keymap_commands_file(),
        autocomplete_contents(registry),
        "Failed to create temporary keymap command list",
        "Failed to write temporary keymap command list",
        "Failed to replace keymap command list"
    );
}

std::vector<std::string> get_runtime_command_names() {
    return runtime_command_registry().registered_names();
}

std::vector<std::string> get_runtime_command_autocomplete_values() {
    return runtime_command_registry().autocomplete_values();
}

static std::string_view trim(std::string_view value) {
    const std::size_t first = value.find_first_not_of(" \t");

    if (first == std::string_view::npos) {
        return {};
    }

    const std::size_t last = value.find_last_not_of(" \t");

    return value.substr(first, last - first + 1);
}

static std::size_t find_action_delimiter(
    std::string_view value
) {
    int parenthesis_depth = 0;
    bool inside_quotes = false;

    for (std::size_t index = 0; index < value.size(); ++index) {
        const char character = value[index];

        if (character == '"' &&
            (index == 0 || value[index - 1] != '\\')) {
            inside_quotes = !inside_quotes;
            continue;
        }

        if (inside_quotes) {
            continue;
        }

        if (character == '(') {
            ++parenthesis_depth;
        }
        else if (character == ')') {
            if (parenthesis_depth == 0) {
                return std::string_view::npos;
            }

            --parenthesis_depth;
        }
        else if (character == ',' &&
            parenthesis_depth == 0) {
            return index;
        }
    }

    return std::string_view::npos;
}

/**
 * \brief Parses one bindings.ini entry.
 * \param line The complete keymap configuration line.
 * \param key_string Receives the configured key name.
 * \param primary Receives the primary command expression.
 * \param secondary Receives the secondary command expression.
 * \return true if the line has a key and a `{primary, secondary}` pair;
 *         action strings may be empty (unbound). Otherwise false.
 */
static bool parse_line(
    std::string_view line,
    std::string_view& key_string,
    std::string_view& primary,
    std::string_view& secondary
) {
    const std::size_t equals = line.find('=');
    if (equals == std::string_view::npos) {
        return false;
    }

    const std::size_t opening_brace =
        line.find('{', equals);

    const std::size_t closing_brace =
        line.rfind('}');

    if (opening_brace == std::string_view::npos ||
        closing_brace == std::string_view::npos ||
        closing_brace <= opening_brace + 1) {
        return false;
    }

    const std::string_view actions = line.substr(
        opening_brace + 1,
        closing_brace - opening_brace - 1
    );

    const std::size_t delimiter =
        find_action_delimiter(actions);

    if (delimiter == std::string_view::npos) {
        return false;
    }

    key_string = trim(line.substr(0, equals));

    primary = trim(actions.substr(0, delimiter));
    secondary = trim(actions.substr(delimiter + 1));

    return !key_string.empty();
}

static std::string display_key_name(std::string_view key_name) {
    std::string display {key_name};
    std::ranges::replace(display, '_', ' ');
    return display;
}

static void report_incorrect_value(
    std::string_view display_name,
    std::string_view expression
) {
    auto_core.logg_and_print(
        "{} is set to an incorrect value: {}",
        display_name,
        expression
    );
}

static command_registry::Action no_op_action() {
    return [] {};
}

static command_registry::Action incorrect_value_action(
    std::string display_name,
    std::string expression
) {
    report_incorrect_value(display_name, expression);
    return [display_name = std::move(display_name),
            expression = std::move(expression)] {
        report_incorrect_value(display_name, expression);
    };
}

static std::filesystem::path journal_choices_file() {
    return ac::paths::journal_directory() / "journal_choices.ini";
}

static std::unordered_map<std::string, std::string> journal_aliases;

static void load_journal_aliases(const command_registry::Registry& registry) {
    journal_aliases.clear();
    ac::config::seed_missing_journal_choices();

    std::ifstream input(journal_choices_file());
    if (!input) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::string_view trimmed_line = trim(line);
        if (trimmed_line.empty() ||
            trimmed_line == "[journal_choices]" ||
            trimmed_line.starts_with(';') ||
            trimmed_line.starts_with('#')) {
            continue;
        }

        const std::size_t equals = trimmed_line.find('=');
        if (equals == std::string_view::npos) {
            auto_core.logg_and_print(
                "Invalid journal alias line format: {}",
                line
            );
            continue;
        }

        const auto name = std::string {
            trim(trimmed_line.substr(0, equals))
        };
        const auto expression = std::string {
            trim(trimmed_line.substr(equals + 1))
        };
        if (name.empty() ||
            expression.empty() ||
            name.find('(') != std::string::npos) {
            auto_core.logg_and_print(
                "Invalid journal alias line format: {}",
                line
            );
            continue;
        }

        if (registry.contains(name)) {
            auto_core.logg_and_print(
                "Journal alias '{}' skipped because that command is reserved",
                name
            );
            continue;
        }

        journal_aliases.insert_or_assign(name, expression);
    }
}

static std::vector<std::string> journal_alias_names() {
    std::vector<std::string> names;
    names.reserve(journal_aliases.size());
    for (const auto& [name, expression] : journal_aliases) {
        names.push_back(name);
    }
    std::ranges::sort(names);
    return names;
}

struct SideBind {
    command_registry::Action action;
    bool resolved = false;
};

static SideBind bind_side(
    std::string_view expression,
    const command_registry::Registry& registry,
    std::string_view display_name
) {
    if (expression.empty()) {
        return { no_op_action(), false };
    }

    command_registry::Action action = registry.resolve(expression);
    if (action) {
        return { std::move(action), true };
    }

    if (expression.find('(') == std::string_view::npos) {
        const auto alias = journal_aliases.find(std::string {expression});
        if (alias != journal_aliases.end()) {
            action = registry.resolve(alias->second);
            if (action) {
                return { std::move(action), true };
            }
            return {
                incorrect_value_action(
                    std::string {display_name},
                    alias->second
                ),
                false
            };
        }
    }

    return {
        incorrect_value_action(
            std::string {display_name},
            std::string {expression}
        ),
        false
    };
}

/**
 * \brief Parses the keymap configuration file and sets the keymap.
 *
 * This function reads bindings.ini, parses each configured key binding,
 * and populates the keymap with primary and secondary actions.
 */
static bool set_keymap_from_stream(
    std::istream& config_file,
    const command_registry::Registry& registry,
    const KeymapSettings& settings
) {
    active_keymap.clear();
    std::string line;
    std::size_t configured_entries = 0;

    if (settings.trace_enabled) {
        auto_core.logg_and_logg("keymap trace logging enabled");
    }

    while (std::getline(config_file, line)) {
        const std::string_view trimmed_line = trim(line);

        if (trimmed_line.empty() ||
            trimmed_line == "[keymap]" ||
            trimmed_line.starts_with(';') ||
            trimmed_line.starts_with('#')) {
            continue;
        }

        std::string_view key_string;
        std::string_view primary;
        std::string_view secondary;

        if (!parse_line(line, key_string, primary, secondary)) {
            auto_core.logg_and_print(
                "Invalid keymap line format: {}",
                line
            );
            continue;
        }

        if (primary.empty() && secondary.empty()) {
            continue;
        }

        const std::optional<key_codes::Code> key =
            key_codes::resolve(key_string);

        if (!key) {
            auto_core.logg_and_print(
                "Unknown keymap key: {}",
                key_string
            );
            continue;
        }

        if (settings.trace_enabled) {
            auto_core.logg_and_logg(
                "creating keymap actions: {} = {}, {}",
                key_string,
                primary,
                secondary
            );
        }

        const std::string display_name = display_key_name(key_string);
        SideBind primary_bind =
            bind_side(primary, registry, display_name);
        SideBind secondary_bind =
            bind_side(secondary, registry, display_name);

        active_keymap[*key] = {
            std::move(primary_bind.action),
            std::move(secondary_bind.action)
        };

        if (primary_bind.resolved || secondary_bind.resolved) {
            ++configured_entries;
        }
    }

    if (configured_entries == 0) {
        auto_core.logg_and_print(
            "Runtime keymap contained no usable entries"
        );
        return false;
    }

    if (!settings.silence_nonset_warning) {
        for (const auto& key : key_codes::keys) {
            if (!active_keymap.contains(key.code)) {
                auto_core.logg_and_print(
                    "{} hasn't been set",
                    display_key_name(key.name)
                );
            }
        }
    }

    auto_core.logg_and_logg("keymap configured from file");
    return true;
}

static bool set_keymap_from_file(
    const command_registry::Registry& registry,
    const KeymapSettings& settings
) {
    load_journal_aliases(registry);

    std::ifstream config_file(ac::paths::keymap_file());

    if (!config_file) {
        auto_core.logg_and_logg(
            "Failed to open runtime keymap: {}",
            ac::paths::keymap_file().string()
        );
        return false;
    }

    return set_keymap_from_stream(config_file, registry, settings);
}

static void set_emergency_keymap() {
    active_keymap.clear();
    active_keymap[key_codes::numpad_0] = {
        activate_function_key,
        deactivate_function_key
    };
    active_keymap[key_codes::numpad_1] = {
        taskbar_pipe_command(
            ac::protocol::taskbar::commands::activate_auto_core
        ),
        close_program
    };
}

void set_keymap_from_file() {
    const command_registry::Registry& registry =
        runtime_command_registry();

    if (!set_keymap_from_file(registry, keymap_settings())) {
        set_emergency_keymap();
    }
}

void initialize_keymap() {
    if (!initialize_keymap_workspace()) {
        auto_core.logg_and_logg(
            "Using emergency keymap because workspace initialization failed"
        );
        set_emergency_keymap();
        return;
    }

    const command_registry::Registry& registry =
        runtime_command_registry();
    load_journal_aliases(registry);
    refresh_journal_keymap_commands();
    refresh_keymap_commands(registry);

    if (!set_keymap_from_file(registry, keymap_settings())) {
        auto_core.logg_and_logg(
            "Using emergency keymap because bindings.ini could not be loaded"
        );
        set_emergency_keymap();
    }
}
